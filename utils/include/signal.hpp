#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <signal.h>

namespace mrpc {
using SigHandler = void (*)(int);

SigHandler signal(int signo, SigHandler handler) {
    struct sigaction act, oact;

    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    } else {
        act.sa_flags |= SA_RESTART;
    }

    if (sigaction(signo, &act, &oact) < 0)
        return (SIG_ERR);
    return (oact.sa_handler);
}
}// namespace mrpc

#endif //SIGNAL_HPP
