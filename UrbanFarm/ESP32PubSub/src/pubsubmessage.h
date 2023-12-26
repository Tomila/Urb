/*
    This file is for formatting the pubsub JSON message

    The format of Pub/Sub message is defined here:
    https://cloud.google.com/pubsub/docs/reference/rest/v1/PubsubMessage
*/

// Pub/Sub message JSON format
static const char* message_format = "{"
    "\"messages\":[{"
        "\"data\": \"%s\","
        "\"ordering_key\": \"first order\","
        "\"attributes\":{"
            "\"kampus\": \"Myyrmaki\"," // Example (format with snprintf)
            "\"huone\": \"MC230\","     // Example (format with snprintf)
            "\"ph\": \"5,9\","          // Example (format with snprintf)
            "\"ec\": \"1.0\","          // Example (format with snprintf)
        "}}]"
    "}";

