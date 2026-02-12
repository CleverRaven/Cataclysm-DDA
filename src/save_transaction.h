#pragma once
#ifndef CATA_SRC_SAVE_TRANSACTION_H
#define CATA_SRC_SAVE_TRANSACTION_H

#include <filesystem>

/**
 * Directory-level transactional save wrapper.
 *
 * Before writing any save files, the constructor hardlinks (or copies) the
 * entire save directory into a ".save_backup" subdirectory.  If the save
 * completes successfully, commit() removes the backup.  If the process
 * crashes or an exception unwinds before commit(), the destructor attempts
 * to restore the backup.  On the next load, recover_if_needed() detects
 * leftover backup directories from interrupted saves and restores them.
 */
class save_transaction
{
    public:
        enum class fsync_level {
            markers_only,   // fsync backup/commit markers + directories (quicksave/autosave)
            full            // fsync every written file + directories (Save & Exit)
        };

        /**
         * Create a backup snapshot of @p save_dir and begin the transaction.
         * Throws std::runtime_error if the backup cannot be created.
         */
        explicit save_transaction( const std::filesystem::path &save_dir,
                                   fsync_level level = fsync_level::markers_only );

        /**
         * If commit() was not called, attempts to restore from backup.
         */
        ~save_transaction();

        save_transaction( const save_transaction & ) = delete;
        save_transaction &operator=( const save_transaction & ) = delete;

        /**
         * Mark the save as complete: write commit marker, fsync, remove backup.
         * Returns false if the commit marker could not be written.
         */
        bool commit();

        /**
         * Call at load time to recover from an interrupted save.
         * If a backup directory exists without a commit marker, the live save
         * is replaced with the backup contents.
         */
        static void recover_if_needed( const std::filesystem::path &save_dir );

        /**
         * Returns true if a transaction is active and its fsync level is 'full'.
         * Called by ofstream_wrapper::close() to decide whether to fsync each file.
         */
        static bool wants_full_fsync();

        /**
         * Record that an fsync operation failed during a full-fsync transaction.
         * Called by fsync_file()/fsync_directory() on failure.  commit() will
         * refuse to finalize if any sync failure was recorded.
         */
        static void notify_sync_failure();

    private:
        std::filesystem::path save_dir_;
        std::filesystem::path backup_dir_;   // save_dir_ / ".save_backup"
        fsync_level level_;
        bool committed_ = false;
        bool backup_created_ = false;
        bool sync_failed_ = false;

        void create_backup();
        void remove_backup();
        void restore_from_backup();

        // NOLINTNEXTLINE(cata-static-int_id-constants)
        static save_transaction *current_;
};

#endif // CATA_SRC_SAVE_TRANSACTION_H
