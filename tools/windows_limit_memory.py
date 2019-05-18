#!/usr/bin/env python3
# -*- coding: utf-8 -*-
""""
Requires: python 3.6+; Windows 7+
Code format: PEP-8; line breaks at 120; Black default formatting.

Limits a target process to a given amount of memory.
If the process use more memory than the given limit, the process allocations will start to fail.

```
$ python .\windows_limit_memory.py --help
usage: windows_limit_memory.py [-h]
                               [-l {NOTSET,DEBUG,INFO,WARNING,ERROR,CRITICAL}]
                               [-m MEMORY]
                               {process,pid} ...

C:DDA Memory Limit test script.

positional arguments:
  {process,pid}         help for sub-commands
    process             Start a new process.
    pid                 Enforce limit on a given process pid.

optional arguments:
  -h, --help            show this help message and exit
  -l {NOTSET,DEBUG,INFO,WARNING,ERROR,CRITICAL}, --log-level {NOTSET,DEBUG,INFO,WARNING,ERROR,CRITICAL}
                        Set the logging level.
  -m MEMORY, --memory MEMORY
                        Maximum process memory size in MiB.
```

Examples:
    ; limiting memory of process with PID 22764 to 50 MiB.
    $ python windows_limit_memory.py -m 50 pid 22764

    ; starting process "cataclysm-tiles.exe" and limiting its memory to 1GiB.
    $ python windows_limit_memory.py -m 1024 process z:\CDDA\Cataclysm-tiles.exe
"""

import argparse
import ctypes
import logging
import pathlib
import platform
import sys
from typing import Any, Dict, Optional

logger = logging.getLogger(__name__)

#
# Windows basic types
#
LPCWSTR = ctypes.c_wchar_p
BOOL = ctypes.c_long
HANDLE = ctypes.c_void_p
WORD = ctypes.c_uint16
DWORD = ctypes.c_uint32
ULONG_PTR = ctypes.c_ulonglong if ctypes.sizeof(ctypes.c_void_p) == 8 else ctypes.c_ulong
SIZE_T = ULONG_PTR
PVOID = ctypes.c_void_p
LPVOID = PVOID
LPDWORD = ctypes.POINTER(DWORD)
PULONG_PTR = ctypes.POINTER(ULONG_PTR)

#
# Windows defines
#
CREATE_SUSPENDED = 0x4
CREATE_BREAKAWAY_FROM_JOB = 0x01000000
INVALID_HANDLE_VALUE = 2 ** (ctypes.sizeof(ctypes.c_void_p) * 8) - 1  # -1 on 32 or 64-bit.
JobObjectAssociateCompletionPortInformation = 7
JobObjectExtendedLimitInformation = 9
JOB_OBJECT_LIMIT_PROCESS_MEMORY = 0x100
PROCESS_SET_QUOTA = 0x100
PROCESS_TERMINATE = 0x1
INFINITE = 0xFFFFFFFF

# Completion Port Messages for job objects
JOB_OBJECT_MSG_END_OF_JOB_TIME = 1
JOB_OBJECT_MSG_END_OF_PROCESS_TIME = 2
JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT = 3
JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO = 4
JOB_OBJECT_MSG_NEW_PROCESS = 6
JOB_OBJECT_MSG_EXIT_PROCESS = 7
JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS = 8
JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT = 9
JOB_OBJECT_MSG_JOB_MEMORY_LIMIT = 10
JOB_OBJECT_MSG_NOTIFICATION_LIMIT = 11
JOB_OBJECT_MSG_JOB_CYCLE_TIME_LIMIT = 12
JOB_OBJECT_MSG_SILO_TERMINATED = 13

#
# Windows Structures
#


class SECURITY_ATTRIBUTES(ctypes.Structure):
    _fields_ = [("nLength", DWORD), ("lpSecurityDescriptor", LPVOID), ("bInheritHandle", BOOL)]


class STARTUPINFO(ctypes.Structure):
    _fields_ = [
        ("cb", DWORD),
        ("lpReserved", LPCWSTR),
        ("lpDesktop", LPCWSTR),
        ("lpTitle", LPCWSTR),
        ("dwX", DWORD),
        ("dwY", DWORD),
        ("dwXSize", DWORD),
        ("dwYSize", DWORD),
        ("dwXCountChars", DWORD),
        ("dwYCountChars", DWORD),
        ("dwFillAttribute", DWORD),
        ("dwFlags", DWORD),
        ("wShowWindow", WORD),
        ("cbReserved2", WORD),
        ("lpReserved2", LPVOID),
        ("hStdInput", HANDLE),
        ("hStdOutput", HANDLE),
        ("hStdError", HANDLE),
    ]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", HANDLE), ("hThread", HANDLE), ("dwProcessId", DWORD), ("dwThreadId", DWORD)]


class IO_COUNTERS(ctypes.Structure):
    _fields_ = [
        ('ReadOperationCount', ctypes.c_ulonglong),
        ('WriteOperationCount', ctypes.c_ulonglong),
        ('OtherOperationCount', ctypes.c_ulonglong),
        ('ReadTransferCount', ctypes.c_ulonglong),
        ('WriteTransferCount', ctypes.c_ulonglong),
        ('OtherTransferCount', ctypes.c_ulonglong),
    ]


class JOBOBJECT_BASIC_LIMIT_INFORMATION(ctypes.Structure):
    _fields_ = [
        ('PerProcessUserTimeLimit', ctypes.c_int64),
        ('PerJobUserTimeLimit', ctypes.c_int64),
        ('LimitFlags', ctypes.c_uint32),
        ('MinimumWorkingSetSize', ctypes.c_ulonglong),
        ('MaximumWorkingSetSize', ctypes.c_ulonglong),
        ('ActiveProcessLimit', ctypes.c_uint32),
        ('Affinity', ctypes.c_void_p),
        ('PriorityClass', ctypes.c_uint32),
        ('SchedulingClass', ctypes.c_uint32),
    ]


class JOBOBJECT_EXTENDED_LIMIT_INFORMATION(ctypes.Structure):
    _fields_ = [
        ('BasicLimitInformation', JOBOBJECT_BASIC_LIMIT_INFORMATION),
        ('IoInfo', IO_COUNTERS),
        ('ProcessMemoryLimit', ctypes.c_ulonglong),
        ('JobMemoryLimit', ctypes.c_ulonglong),
        ('PeakProcessMemoryUsed', ctypes.c_ulonglong),
        ('PeakJobMemoryUsed', ctypes.c_ulonglong),
    ]


class JOBOBJECT_ASSOCIATE_COMPLETION_PORT(ctypes.Structure):
    _fields_ = [('CompletionKey', LPVOID), ('CompletionPort', HANDLE)]


class OVERLAPPED(ctypes.Structure):
    _fields_ = [("Internal", ULONG_PTR), ("InternalHigh", ULONG_PTR), ("Pointer", LPVOID), ("hEvent", HANDLE)]


LPSECURITY_ATTRIBUTES = ctypes.POINTER(SECURITY_ATTRIBUTES)
LPSTARTUPINFO = ctypes.POINTER(STARTUPINFO)
LPPROCESS_INFORMATION = ctypes.POINTER(PROCESS_INFORMATION)
LPOVERLAPPED = ctypes.POINTER(OVERLAPPED)


class Kernel32Wrapper:
    """A class used to encapsulate kernel32 library functions.
    """

    def __init__(self) -> None:
        """Initialization.
        """
        # get kernel32 instance
        self._kernel32: ctypes.WinDLL = ctypes.WinDLL("kernel32", use_last_error=True)

        #
        # fetch functions
        #

        self._create_job_object = self._kernel32.CreateJobObjectW
        self._create_job_object.argtypes = (LPSECURITY_ATTRIBUTES, LPCWSTR)
        self._create_job_object.restype = HANDLE

        self._query_information_job_object = self._kernel32.QueryInformationJobObject
        self._query_information_job_object.argtypes = (HANDLE, ctypes.c_uint32, LPVOID, DWORD, ctypes.POINTER(DWORD))
        self._query_information_job_object.restype = BOOL

        self._set_information_job_object = self._kernel32.SetInformationJobObject
        self._set_information_job_object.argtypes = (HANDLE, ctypes.c_uint32, LPVOID, DWORD)
        self._set_information_job_object.restype = BOOL

        self._assign_process_to_job_object = self._kernel32.AssignProcessToJobObject
        self._assign_process_to_job_object.argtypes = (HANDLE, HANDLE)
        self._assign_process_to_job_object.restype = BOOL

        self._create_process = self._kernel32.CreateProcessW
        self._create_process.argtypes = (
            LPCWSTR,
            LPCWSTR,
            LPSECURITY_ATTRIBUTES,
            LPSECURITY_ATTRIBUTES,
            BOOL,
            DWORD,
            LPVOID,
            LPCWSTR,
            LPSTARTUPINFO,
            LPPROCESS_INFORMATION,
        )
        self._create_process.restype = BOOL

        self._open_process = self._kernel32.OpenProcess
        self._open_process.argtypes = (DWORD, BOOL, HANDLE)
        self._open_process.restype = HANDLE

        self._create_io_completion_port = self._kernel32.CreateIoCompletionPort
        self._create_io_completion_port.argtypes = (HANDLE, HANDLE, ULONG_PTR, DWORD)
        self._create_io_completion_port.restype = HANDLE

        self._get_queued_completion_status = self._kernel32.GetQueuedCompletionStatus
        self._get_queued_completion_status.argtypes = (HANDLE, LPDWORD, PULONG_PTR, ctypes.POINTER(LPOVERLAPPED), DWORD)
        self._get_queued_completion_status.restype = BOOL

        self._resume_thread = self._kernel32.ResumeThread
        self._resume_thread.argtypes = (HANDLE,)
        self._resume_thread.restype = DWORD

        self._terminate_process = self._kernel32.TerminateProcess
        self._terminate_process.argtypes = (HANDLE, ctypes.c_uint32)
        self._terminate_process.restype = BOOL

        self._close_handle = self._kernel32.CloseHandle
        self._close_handle.argtypes = (HANDLE,)
        self._close_handle.restype = BOOL

        self._function_map = {
            "AssignProcessToJobObject": self._assign_process_to_job_object,
            "CloseHandle": self._close_handle,
            "CreateIoCompletionPort": self._create_io_completion_port,
            "CreateJobObject": self._create_job_object,
            "CreateProcess": self._create_process,
            "GetQueuedCompletionStatus": self._get_queued_completion_status,
            "OpenProcess": self._open_process,
            "QueryInformationJobObject": self._query_information_job_object,
            "ResumeThread": self._resume_thread,
            "TerminateProcess": self._terminate_process,
            "SetInformationJobObject": self._set_information_job_object,
        }

    def __getattr__(self, item: str):
        """Get an attribute from the class.

        Args:
            item: The attribute to get.

        Raises:
            AttributeError: the given name is not a known function name.

        Returns:
            If the attribute is a function name, returns the function pointer, otherwise raise an exception.
        """
        func = self._function_map.get(item)
        if func is None:
            raise AttributeError(item)
        return func

    @staticmethod
    def create_buffer(obj: Any, max_buffer_len: Optional[int] = None) -> str:
        """Creates a ctypes unicode buffer given an object convertible to string.

        Args:
            obj: The object from which to create a buffer. Must be convertible to `str`.
            max_buffer_len: The maximum buffer length for the buffer. If `None` is supplied, default to the length of
              the given object string.

        Returns:
            A unicode buffer of object converted to string.
        """
        str_obj = obj if isinstance(obj, str) else str(obj)

        max_len_value = len(str_obj) + 1 if max_buffer_len is None else max_buffer_len
        max_len = max(max_len_value, len(str_obj))

        return ctypes.create_unicode_buffer(str_obj, max_len)


class ProcessLimiter:
    """A class used to limit a process memory using Windows Jobs.
    """

    def __init__(self) -> None:
        """Initialization.
        """
        logger.debug("instantiating kernel32 wrapper.")
        self._kernel32: Kernel32Wrapper = Kernel32Wrapper()
        self._handle_process: Optional[HANDLE] = None
        self._handle_thread: Optional[HANDLE] = None
        self._handle_job: Optional[HANDLE] = None
        self._handle_io_port: Optional[HANDLE] = None
        try:
            self._handle_io_port: HANDLE = self._create_io_completion_port()
        except ctypes.WinError:
            pass

    def __enter__(self) -> "ProcessLimiter":
        """Context manager entry.
        """
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """Context manager exit.

        Args:
            exc_type: exception type.
            exc_val: exception value.
            exc_tb: Exception trackeback.
        """
        if self._handle_process:
            self._kernel32.CloseHandle(self._handle_process)
            self._handle_process = None
        if self._handle_thread:
            self._kernel32.CloseHandle(self._handle_thread)
            self._handle_thread = None
        if self._handle_io_port:
            self._kernel32.CloseHandle(self._handle_io_port)
            self._handle_io_port = None
        if self._handle_job:
            self._kernel32.CloseHandle(self._handle_job)
            self._handle_job = None

    @property
    def has_io_port(self) -> bool:
        """Get whether the current class instance holds a Windows I/O port.

        Returns:
            `True` if the class hold an I/O port, `False` otherwise.
        """
        return self._handle_io_port is not None

    @property
    def is_started_process(self) -> bool:
        """Get whether the process object was started by the class or not.

        Returns:
            True if the process was started by this class instance, False otherwise.
        """
        return self._handle_process is not None and self._handle_thread is not None

    def _create_io_completion_port(self) -> HANDLE:
        """[Internal] Create an I/O completion port.

        Notes:
            The I/O port is used later in the `wait_for_job()` function.

        Raises:
            ctypes.WinError: An error occurred while creating the I/O port.

        Returns:
            A `HANDLE` to the I/O Port.
        """
        logger.info("Creating IO Port")
        handle_io_port = self._kernel32.CreateIoCompletionPort(INVALID_HANDLE_VALUE, None, 0, 1)
        if not handle_io_port:
            raise ctypes.WinError(ctypes.get_last_error())
        logger.debug(f"IO Port: {handle_io_port:#x}")
        return handle_io_port

    def _query_information_job_object(self, structure: ctypes.Structure, query_type: int) -> ctypes.Structure:
        """[Internal] Retrieves limit and job state information from the job object.

        Args:
            structure: The limit or job state information.
            query_type: The information class for the limits to be queried.

        Raises:
            ctypes.WinError: An error occurred while getting the state or limit for the job object.

        Returns:
            Returns the given structure filled with the job limit or state information.
        """
        # query its default properties
        return_length = DWORD(0)
        logger.debug("Querying job object.")
        ret_val = self._kernel32.QueryInformationJobObject(
            self._handle_job, query_type, ctypes.byref(structure), ctypes.sizeof(structure), ctypes.byref(return_length)
        )
        if ret_val == 0 or return_length.value != ctypes.sizeof(structure):
            raise ctypes.WinError(ctypes.get_last_error())
        return structure

    def _resume_main_thread(self) -> None:
        """[Internal] Resume the main thread of a created process.

        Raises:
            ValueError: the function was called but there is no main thread handle.
            ctypes.WinError: There was an error while trying to resume the main thread of the process. Note that this is
                a critical condition resulting in a zombie process. The code will try to kill the zombie process if it
                happens, without any guaranty of success.
        """
        if not self._handle_thread:
            raise ValueError("Thread handle is NULL.")
        # resume the main thread and let the process run.
        logger.debug("Resuming main thread.")
        ret_val = self._kernel32.ResumeThread(self._handle_thread)
        if ret_val == -1:
            # oops, we now have a zombie process... we'll try to kill it nonetheless.
            logger.error("Error: ResumeThread failed but the process is started!")
            # try to kill it
            if self._kernel32.TerminateProcess(self._handle_process, 0xDEADBEEF) != 0:
                logger.info("Successfully killed the zombie process.")
            else:
                logger.warning("The zombie process is sitll alive. Try to kill it manually.")
            # we tried to kill the process, now raise.
            raise ctypes.WinError(ctypes.get_last_error())

    def _set_information_job_object(self, structure: ctypes.Structure, set_type: int) -> None:
        """[Internal] Set limit and job state information for the job object.

        Args:
            structure: The limit or job state information.
            set_type: The information class for the limits to be queried.

        Raises:
            ctypes.WinError: An error occurred while setting the state or limit for the job object.
        """
        ret_val = self._kernel32.SetInformationJobObject(
            self._handle_job, set_type, ctypes.byref(structure), ctypes.sizeof(structure)
        )
        if ret_val == 0:
            raise ctypes.WinError(ctypes.get_last_error())

    def assign_process_to_job(self) -> None:
        """Assign a process to a job.

        Raises:
            ValueError: There's no process or job to associate with.
            ctypes.WinError: There was an error while associating the process with the job.
        """
        if not self._handle_process:
            raise ValueError("There's no process to associate with the job.")
        if not self._handle_job:
            raise ValueError("There's no job.")
        logger.info("Assigning process to job.")
        ret_val = self._kernel32.AssignProcessToJobObject(self._handle_job, self._handle_process)
        if ret_val == 0:
            raise ctypes.WinError(ctypes.get_last_error())

    def create_job(self, job_name: Optional[str] = None) -> None:
        """Create a job.

        Args:
            job_name: The optional job name; can be `None`.

        Notes:
            This function will try to associate the job with an I/O completion port, which is used later in the
                 `wait_for_job` function.

        Raises:
            ctypes.WinError: There was an error while creating the job, or, if a completion port exists, an error
                occurred while associating the completion port to the job.
        """
        logger.info("Creating job object.")
        handle_job = self._kernel32.CreateJobObject(None, job_name)
        if handle_job == 0:
            raise ctypes.WinError(ctypes.get_last_error())
        logger.debug(f"Job object: {handle_job:#x}")
        self._handle_job = handle_job

        # associate io port completion, if any
        if not self._handle_io_port:
            return
        job_completion_port = JOBOBJECT_ASSOCIATE_COMPLETION_PORT()
        job_completion_port.CompletionKey = self._handle_job
        job_completion_port.CompletionPort = self._handle_io_port
        self._set_information_job_object(job_completion_port, JobObjectAssociateCompletionPortInformation)

    def create_process(self, process_path: pathlib.Path, command_line: Optional[str] = None) -> None:
        """Create a new process object to be associated with the main job.

        Args:
            process_path: The path to the main binary executable.
            command_line: The command line for the process.

        Raises:
            ctypes.WinError: An error occurred while trying to create the process.

        Notes:
            The command line is prepended with the full binary path.
        """
        # create the process with its main thread in a suspended state
        logger.debug("Creating suspended process.")
        si = STARTUPINFO()
        si.cb = ctypes.sizeof(STARTUPINFO)
        pi = PROCESS_INFORMATION()

        full_proc_path = process_path.resolve()
        cmd_line_str = str(full_proc_path) + (command_line if command_line is not None else "")

        cmd_line = self._kernel32.create_buffer(cmd_line_str)
        current_dir = self._kernel32.create_buffer(str(full_proc_path.parent))

        ret_val = self._kernel32.CreateProcess(
            None,  # lpApplicationName
            cmd_line,  # lpCommandLine
            None,  # lpProcessAttributes
            None,  # lpThreadAttributes
            False,  # bInheritHandles
            CREATE_SUSPENDED,  # dwCreationFlags
            None,  # lpEnvironment
            current_dir,  # lpCurrentDirectory
            ctypes.byref(si),  # lpStartupInfo
            ctypes.byref(pi),  # lpProcessInformation
        )
        if ret_val == 0:
            raise ctypes.WinError(ctypes.get_last_error())
        logger.debug(f"Process: {pi.hProcess:#x}; Thread: {pi.hThread:#x}")

        self._handle_process = pi.hProcess
        self._handle_thread = pi.hThread

    def get_process(self, pid: int) -> None:
        """Get a process object from its process identifier (PID). This process will be associated with the main job.

        Args:
            pid: The pid of the process to associate with the job.

        Raises:
            ctypes.WinError: An error occurred while trying to ge the process object form its PID. Ensure you have
                sufficient rights upon the process.
        """
        handle_process = self._kernel32.OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, False, pid)
        if not handle_process:
            raise ctypes.WinError(ctypes.get_last_error())
        logger.debug(f"Process: {handle_process:#x}")
        self._handle_process = handle_process

    def limit_process_memory(self, memory_limit: int) -> None:
        """Effectively limit the process memory that the target process can allocates.

        Args:
            memory_limit: The memory limit of the process, in MiB (MebiBytes).

        Raises:
            ctypes.WinError: There was an error while trying to limit the process memory.
            ValueError: The given memory limit is not valid or there's no job.
        """
        logger.info(f"Limiting process memory to {memory_limit} MiB ({memory_limit * 1024 * 1024} bytes)")
        if memory_limit <= 0:
            raise ValueError(f"Memory limit can be 0 or negative; got: {memory_limit}")
        if not self._handle_job:
            raise ValueError("Job handle is NULL.")
        # query current job object
        job_info = JOBOBJECT_EXTENDED_LIMIT_INFORMATION()
        job_info = self._query_information_job_object(job_info, JobObjectExtendedLimitInformation)

        # limit the process memory; note: not the job memory!
        logger.debug("Setting job information.")
        job_info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY
        job_info.ProcessMemoryLimit = memory_limit * 1024 * 1024  # MiB to bytes
        self._set_information_job_object(job_info, JobObjectExtendedLimitInformation)

    def wait_for_job(self) -> bool:
        """Wait for the job completion. This function returns when the last process of the job exits.

        Notes:
            This function returns immediately if there's no IO ports.

        Returns:
            True if the function successfully waited for the job to finish, False otherwise.
        """
        logger.info("Waiting for the job.")
        if self.is_started_process:
            # resume the main thread, as we started the process ourselves.
            self._resume_main_thread()
        if not self.has_io_port:
            return False
        msg_map: Dict[int, str] = {
            JOB_OBJECT_MSG_END_OF_JOB_TIME: "End of job time",
            JOB_OBJECT_MSG_END_OF_PROCESS_TIME: "End of process time",
            JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT: "Active process limit reached",
            JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO: "No more active process in job",
            JOB_OBJECT_MSG_NEW_PROCESS: "New process in job",
            JOB_OBJECT_MSG_EXIT_PROCESS: "A process in the job exited",
            JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS: "A process in the job exited abnormally",
            JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT: "A process in the job reached its memory limit",
            JOB_OBJECT_MSG_JOB_MEMORY_LIMIT: "The job has reached its memory limit",
            JOB_OBJECT_MSG_NOTIFICATION_LIMIT: "The job reached a notification limit",
            JOB_OBJECT_MSG_JOB_CYCLE_TIME_LIMIT: "The CPU cycle limit for the job has been reached",
            JOB_OBJECT_MSG_SILO_TERMINATED: "A silo a terminated.",
        }

        return_val = False

        completion_code = DWORD(0)
        completion_key = ULONG_PTR(0)
        overlapped = OVERLAPPED()
        lp_overlapped = ctypes.POINTER(OVERLAPPED)(overlapped)
        while True:
            ret_val = self._kernel32.GetQueuedCompletionStatus(
                self._handle_io_port,
                ctypes.byref(completion_code),
                ctypes.byref(completion_key),
                ctypes.byref(lp_overlapped),
                INFINITE,
            )
            if ret_val == 0:
                # error from GetQueuedCompletionStatus
                logger.error(f"Error GetQueuedCompletionStatus: {ctypes.get_last_error():#x}")
                break
            if completion_key.value != self._handle_job:
                # we received an event, but it's not from our job; we can ignore it!
                continue
            # message
            msg = msg_map.get(completion_code.value)
            if msg:
                logger.info(f"IO Port Message: {msg}")
            if completion_code.value == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
                # no more processes in the job, we can exit.
                return_val = True
                break
        logger.info("Job wait finished")
        return return_val


def main(args: argparse.Namespace) -> int:
    # runs only on Windows
    if platform.system().lower() != "windows":
        logger.error("This script only works on Microsoft Windows systems.")
        return -1

    # check the memory limit
    if args.memory <= 0:
        logger.debug(f"Memory limit must be above 0, got: {args.memory}")
        return -1

    # check the command type
    if args.command_name == "process":
        # check the binary file exists
        if not args.process.is_file():
            logger.debug(f"The given file path '{args.process}' is not a file or doesn't exist.")
            return -1
        logger.info(f"Process Path: {args.process}; Memory limit (MiB): {args.memory}")
    elif args.command_name == "pid":
        # pid to int
        base = 16 if args.pid.startswith("0x") or args.pid.startswith("0X") else 10
        args.pid = int(args.pid, base)
        logger.info(f"Process Pid: {args.pid:#x}; Memory limit (MiB): {args.memory}")
    else:
        logger.error(f"Unknown command: '{args.command}'")
        return -1

    with ProcessLimiter() as proc_limiter:
        proc_limiter.create_job("MEMORY_LIMITER_JOB")

        if args.command_name == "process":
            proc_limiter.create_process(args.process)
        else:
            proc_limiter.get_process(args.pid)
        proc_limiter.assign_process_to_job()
        proc_limiter.limit_process_memory(args.memory)
        if not proc_limiter.wait_for_job():
            input("Press <enter> to exit this script")

    print("Script done.")
    return 0


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="C:DDA Memory Limit test script.")

    #
    # options for all sub-commands
    #
    arg_parser.add_argument(
        "-l",
        "--log-level",
        choices=['NOTSET', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
        default='INFO',
        help="Set the logging level.",
    )
    arg_parser.add_argument(
        "-m", "--memory", action="store", type=int, default=1024, help="Maximum process memory size in MiB."
    )

    #
    # sub-commands
    #
    subparsers = arg_parser.add_subparsers(help='help for sub-commands', dest="command_name")

    parser_new_process = subparsers.add_parser('process', help='Start a new process.')
    parser_new_process.add_argument("process", action="store", type=pathlib.Path, help="Path to process binary file.")

    parser_pid = subparsers.add_parser('pid', help='Enforce limit on a given process pid.')
    parser_pid.add_argument("pid", action="store", type=str, help="Process pid on which to enforce memory limit.")

    parsed_args = arg_parser.parse_args()

    if not parsed_args.command_name:
        arg_parser.error("'process' or 'pid' command is required.")
    logging_level = logging.getLevelName(parsed_args.log_level)
    logging.basicConfig(level=logging_level)
    logger.setLevel(logging_level)

    sys.exit(main(parsed_args))
