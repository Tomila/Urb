//Create a copy of this file and change the name to config.h
//change the fields to your needs. 
//You must have created the service account for google cloud in order to fill the jwt and pubsub parts. 


// WiFi settings. we recommand using your phones hotspot feature instead of metropolia wifi. If you use Iphone turn on the "Maximise campatibility". Other problem apple users may encounter is that the name does not match
// This is caused by a diffrence in the "´" symbol in the wifi name. if you encounter this issue just join to the network with you laptop and copy paste the name from windows wifi properties.
#define WIFI_SSID   "esp31"         //Name of the wifi
#define WIFI_PASS   "123asd124"     //Password
#define MAX_RETRY   10              //Maximum amount of retries to connect before the Esp errors

// PubSub
#define PUBSUB_URL  "https://pubsub.googleapis.com/v1/projects/innoprojekti/topics/sensordata:publish" //url where esp sends pubsub messages to. 

// JWT
//Private key of the service account. This must be acquired from google cloud. One service account can have multiple keyfiles. 
//The steps to acquire this is explained in the setup part of github wiki.
//They keyfile is json file that contains information about the account aswell as the private key required for authentication to Oauth2. The keyfile by its self is not enought to publish to pubsub.
#define PRIVATE_KEY "-----BEGIN PRIVATE KEY-----\nywak2QKBgQC+C9cI0tQsNGso8i6XZlu5fQr+SUQ0T0bd\ndMYH34jkhWxSV+PyM+O1rcplSJaRic+v88cAunfDuw/id0UdK1XvoMq3VPMhQGKF\nflnEgSGHs8TFp48DgXILc0YJaBVRflz37TR/6Ets32iN3Q5FqHJJIB+Ohd3n37EG\nerymNQZoNQKBgG8OGvNshkuHxWZhCsGbxD6ezGajskaeN+xPcpJtUx4JsBfVAumw\nH6HKKsDTIwIKoQRZJ0lHind5y2iCpeX9jyrDtXVswLTjsNeErNuM5j89c3pIVUdT\neYXVMrZaeRUSb4RnSiKQ0pckHAxVY/OXEMkF8Bj+hqy2WezWK/zoGoBxAoGAaXkA\nurDneDEPWAYwvW3dcjBnrYDUDYQB/F4WmcABpXI3D+wfYghSVD5DmbqTdepaDXjp\n9QmH9mUXB8TR4sJuSzOdzemjlcy54QSipWBVT48BogDAal2pAZnUQNw3GKdcyWwo\nOuSdF6tVx4XrqdU2uEbPvLWm1k1bXhE4KbY7/BECgYBw5kmn10dk7wCTajaZVW4q\n6sfsTEldvNloY1hHXQaiVegRpHtG8W2QeswHIAljSE7Yls3DjgvwCz3ISkeRGU9m\n/hVvusdX0TqNjx/M9IZvbh2hp6aTGzJgNoerV7RaKhD9lWfP5kovlbvzwQVL+5uS\n8dxc6cQSbQMqwk6bJskVPg==\n-----END PRIVATE KEY-----\n"
#define KEY_ID      "e55314eaf2b665abe60"   //Header "kid"
#define ISS         "esp32-623@innoprojekti.iam.gserviceaccount.com" //Payload "iss"
#define SUB         "esp32-623@innoprojekti.iam.gserviceaccount.com" //Payload "sub"

#define TOKEN_EXPIRATION_TIME_IN_SECONDS 3600   // Token expiration time in seconds (can be configured to match sensor reading interval)
