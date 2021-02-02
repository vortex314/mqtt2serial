//____________________________________________________________________________________
//
#include <limero.h>
class Poller : public Actor {
  TimerSource _pollInterval;
  std::vector<Requestable *> _requestables;
  uint32_t _idx = 0;

 public:
  ValueFlow<bool> connected;
  ValueFlow<uint32_t> interval = 500;
  Poller(Thread &t) ;

  template <class T>
  LambdaSource<T> &operator>>(LambdaSource<T> &source) {
    _requestables.push_back(&source);
    return source;
  }

  template <class T>
  ValueSource<T> &operator>>(ValueSource<T> &source) {
    _requestables.push_back(&source);
    return source;
  }

  template <class T>
  ValueFlow<T> &operator>>(ValueFlow<T> &source) {
    _requestables.push_back(&source);
    return source;
  }

  template <class T>
  RefSource<T> &operator>>(RefSource<T> &source) {
    _requestables.push_back(&source);
    return source;
  }
};
