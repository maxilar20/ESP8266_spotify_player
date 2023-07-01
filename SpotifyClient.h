#include <WiFiClientSecure.h>

struct HttpResult
{
    int httpCode;
    String payload;
};

class SpotifyClient
{
public:
    SpotifyClient(String clientId, String clientSecret, String deviceName, String refreshToken);

    void FetchToken();
    int Play(String context_uri);
    void PlaySpotifyUri(String context_uri);
    int Shuffle();
    int Next();
    void GetDevices();

private:
    WiFiClientSecure wifiClient;
    String clientId;
    String clientSecret;
    String redirectUri;
    String accessToken;
    String refreshToken;
    String deviceId;
    String deviceName;

    String ParseJson(String key, String json);
    HttpResult CallAPI(String method, String url, String body);
    String GetDeviceId(String json);
};