#ifndef REALTIMEKIT_H
#define REALTIMEKIT_H

#include <giomm/dbusproxy.h>
#include <glibmm/refptr.h>
#include <iostream>

#define RTKIT_SERVICE_NAME "org.freedesktop.RealtimeKit1"
#define RTKIT_OBJECT_PATH "/org/freedesktop/RealtimeKit1"

class RealtimeKit {
 public:
  RealtimeKit();

  void set_priority(const std::string& source_name, const int& priority);
  void set_nice(const std::string& source_name, const int& nice_value);

  enum PriorityType {
      pt_niceness,
      pt_realtime,
      pt_none
  };

 private:
  Glib::RefPtr<Gio::DBus::Proxy> proxy;
  Glib::RefPtr<Gio::DBus::Proxy> properties_proxy;

  auto get_int_property(const char* propname) -> long long;

  void make_realtime(const std::string& source_name, const int& priority);

  void make_high_priority(const std::string& source_name, const int& nice_value);
};

#endif
