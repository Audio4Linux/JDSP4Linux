# A wrapper for the Qt Network Access API

This is just a wrapper around Qt's QNetworkAccessManager and friends. I use it in my apps at https://flavio.tordini.org . It has a simpler, higher-level API and some functionality not found in Qt:

- Throttling (as required by many web APIs nowadays)
- Automatic retries
- User agent and request header defaults
- Partial requests
- Easier POST requests
- Read timeouts (don't let your requests get stuck forever). (now supported by Qt >= 5.15)
- Redirection support (now supported by Qt >= 5.6)
- Disk-based cache implementation similar to Qt's but not strictly a HTTP cache, i.e. it ignores HTTP headers. This is good if want to cache successful requests irrespective of what the origin server says you should do. The cache also fallbacks to stale content when the server returns an error.

## Design

This library uses the [Decorator design pattern](https://en.wikipedia.org/wiki/Decorator_pattern) to modularize features and make it easy to add them and use them as needed. The main class is [Http](https://github.com/flaviotordini/http/blob/master/src/http.h), which implements the base features of a HTTP client. More specialized classes are:

- [CachedHttp](https://github.com/flaviotordini/http/blob/master/src/cachedhttp.h), a simple disk-based cache
- [ThrottledHttp](https://github.com/flaviotordini/http/blob/master/src/throttledhttp.h), implements request throttling (aka limiting)

The constructor of these classes takes another Http instance for which they will act as a proxy. (See examples below). Following this design you can create your own Http subclass. For example, a different caching mechanism, an event dispatcher, custom request logging, etc.


## Build Instructions
In order to build this library you can use either `qmake` or `cmake`.

### qmake
```
mkdir build
cd build
qmake ..
make
```

### CMake
```
mkdir build
cd build
cmake ..
make
```

## Integration

You can use this library as a git submodule. For example, add it to your project inside a lib subdirectory:

```
git submodule add -b master https://github.com/flaviotordini/http lib/http
```

Then you can update your git submodules like this:

```
git submodule update --init --recursive --remote
```

To integrate the library in your qmake based project just add this to your .pro file:

```
include(lib/http/http.pri)
```

qmake builds all object files in the same directory. In order to avoid filename clashes use:

```
CONFIG += object_parallel_to_source
```

If you are using CMake you can integrate the library by adding the following lines to your CMakeLists.txt:

```
add_subdirectory(lib/http)
...
target_link_library(your_super_cool_project <other libraries> http)
```
or if you have installed http you can find it via:

```
find_library(http REQUIRED)
...
target_link_library(your_super_cool_project <other libraries> http)
```


## Examples

A basic C++14 example:

```
#include "http.h"

auto reply = Http::instance().get("https://google.com/");
connect(reply, &HttpReply::finished, this, [](auto &reply) {
    if (reply.isSuccessful()) {
        qDebug() << "Feel the bytes!" << reply.body();
    } else {
        qDebug() << "Something's wrong here" << reply.statusCode() << reply.reasonPhrase();
    }
});
```

Or using two separate signals for success and failure:
```
#include "http.h"

auto reply = Http::instance().get("https://google.com/");
connect(reply, &HttpReply::data, this, [](auto &bytes) {
    qDebug() << "Feel the bytes!" << bytes;
});
connect(reply, &HttpReply::error, this, [](auto &msg) {
    qDebug() << "Something's wrong here" << msg;
});
```

This is a real-world example of building a Http object with more complex features. It throttles requests, uses a custom user agent and caches results:

```
#include "http.h"
#include "cachedhttp.h"
#include "throttledhttp.h"

Http &myHttp() {
    static Http *http = [] {
        Http *http = new Http;
        http->addRequestHeader("User-Agent", userAgent());

        ThrottledHttp *throttledHttp = new ThrottledHttp(*http);
        throttledHttp->setMilliseconds(1000);

        CachedHttp *cachedHttp = new CachedHttp(*throttledHttp, "mycache");
        cachedHttp->setMaxSeconds(86400 * 30);

        return cachedHttp;
    }();
    return *http;
}
```

If the full power (and complexity) of QNetworkReply is needed you can always fallback to it:

```
#include "http.h"

HttpRequest req;
req.url = "https://flavio.tordini.org/";
QNetworkReply *reply = Http::instance().networkReply(req);
// Use QNetworkReply as needed...
```

Note that features like redirection, retries and read timeouts won't work in this mode.

## License

You can use this library under the MIT license and at your own risk. If you do, you're welcome contributing your changes and fixes.
