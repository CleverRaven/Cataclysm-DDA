#pragma once

namespace snmalloc
{
  /**
   * This class provides a default implementation for checking for
   * initialisation. It does nothing. This is used to allow the thread local
   * state to inject code for correctly initialising the thread local state with
   * an allocator, but this check is performed off the fast path.
   */
  class CheckInitNoOp
  {
  public:
    /**
     * @brief
     *
     * @tparam Success - Lambda type that is called if initialised
     * @tparam Restart - Lambda type that is called if the allocator was not
     * initialised, and has now been initialised.
     * @tparam Args - Arguments to pass to the Restart lambda.
     * @param s - the actual success lambda.
     * @param r - the restart lambda - ignored in this case.
     * @param args - arguments to pass to the restart lambda - ignored in this
     * case.
     * @return auto - the result of the success or restart lambda
     */
    template<typename Success, typename Restart, typename... Args>
    static auto check_init(Success s, Restart, Args...)
    {
      return s();
    }
  };
} // namespace snmalloc