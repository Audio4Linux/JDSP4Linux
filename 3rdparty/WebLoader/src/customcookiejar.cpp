#include "customcookiejar.h"

CustomCookieJar::CustomCookieJar()
{

}
QList<QNetworkCookie> CustomCookieJar::getCookies() const
{
     return this->allCookies();
}
