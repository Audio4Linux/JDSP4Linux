# WebLoader
High level wrapper around QNetworkAccessManager for make network communications easy.

## About
Library allow you to load everything from internet (web pages, images, archives etc.). You can choose how data will be loaded: asynchronously or synchronously.

Library build on top of queue of loaders, everyone of which is simple wrapper around QNetworkAccessManager object. It means that you don't need to warn about memory management, mime types detecting or something else, library does it instead of you.

Based on Qt5.

## How to use
Generic class for interacting with web is NetworkRequest. You can send get, post and raw requests. You can manage referrer, cookies, handle loading progress and of course loading errors for every request.

And we have little helper for common tasks. It named NetworkRequestLoader. It provide simple way to user for download data from internet.

#### #include \<NetworkRequestLoader.h\>
Let's see how easy to load web-pages via NetworkRequestLoader.

Load data synchronously.
```c++
const QByteArray data = NetworkRequestLoader::loadSync("https://github.com");
```
Now let's load data in asynchronous way.
```c++
NetworkRequestLoader::loadAsync("https://github.com", [] (const QByteArray& _loadedData) {
    qDebug() << "Loaded" << _loadedData.size() << "bytes.";
});
```
If you need to load thousands of web-pages this is not a problem - just load them!:)
```c++
NetworkRequestLoader::loadAsync(thousandsUrls, [] (const QByteArray& _loadedData, const QUrl& _fromUrl) {
    qDebug() << "Loaded" << _loadedData.size() << "bytes from" << _fromUrl;
});
```
Don't worry about anything, library makes all work instead of you.

#### #include \<NetworkRequest.h\>
If you need to more complex management of loading process, NetworkRequest helps you.

Let's send some post request.
```c++
NetworkRequest request;
requset.setRequestMethod(NetworkRequest::Post);
request.addRequestAttribute("id", 1893);
request.addRequestAttributeFile("photo", "/home/user/Images/photo.png");
const QByteArray postStatus = request.loadSync("https://site.com/API/v1/savePhoto/");
```
Okey. Now let's send json data to our server.
```c++
NetworkRequest request;
requset.setRequestMethod(NetworkRequest::Post);
request.setRawRequest("{ \"method\": \"getUsers\" }", "application/json");
const QByteArray usersJson = request.loadSync("https://site.com/API/v2/function/");
```
Want to save session cookies? Just sent instance of QNetworkCookieJar class to NetworkRequest.
```c++
QNetworkCookieJar cookies;
NetworkRequest request;
request.setCookieJar(&cookies);
request.loadSync("https://site.com/API/v1/startSession");

// And in next step you can reuse this request or create new NetworkRequest and sent cookies to them

request.loadSync("https://site.com/API/v1/uploadImage");
```
It's really simple, just try!

## Contribution
We really love feedback. If you have ideas for make it better, or find some bugs, or fix some bugs :), or just want to ask question - you welcome!

## Discussion

* https://forum.qt.io/topic/70627/high-level-wrapper-around-qnetworkaccessmanager-for-make-network-communications-easy
* http://www.forum.crossplatform.ru/index.php?showtopic=10833
* http://www.prog.org.ru/topic_30546_0.html

## Credits

Alexey Polushkin @armijo38 and Dimka Novikov @dimkanovikov
