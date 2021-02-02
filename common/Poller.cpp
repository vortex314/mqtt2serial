//____________________________________________________________________________________
//
#include <Poller.h>

Poller::Poller(Thread &t) : Actor(t), _pollInterval(t, 500, true)
{
  _pollInterval >> [&](const TimerMsg tm) {
    if (_requestables.size() && connected())
      _requestables[_idx++ % _requestables.size()]->request();
  };
  interval >> [&](const uint32_t iv) { _pollInterval.interval(iv); };
};
