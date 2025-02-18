/**
 * 
 * Transport NSW Bus Fetcher
 * 
 * Currently a binary, to be used as a library for a future project...
 * 
 * By Thribhu Krishnan || 18/02/25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

#define REFRESH_TIME 60

typedef struct {
    const char* API_KEY;
    char* STOP_ID;
} ENV;

typedef struct {
    char* bus_num;
    char* bus_destination;
    // Time
    char* scheduled;
    // Time
    char* estimated;
    int delay; // Can be + or -
} BusInfo_t;

struct MemoryStruct {
    char* memory;
    size_t size;
};

/**
 * This function loads the '.env' file into 
 * a struct `ENV`. 
 */
ENV* loadEnv(const char* fileName) {
    // Open file to ptr
    FILE *f_ptr;
    f_ptr = fopen(fileName, "r");
    // Check if file is null or doesn't exist
    if (f_ptr == NULL) {
        fprintf(stderr, "Error opening file\n");
        return NULL;
    }

    // Read the api key first time
    char api_key[256];
    // Error checking
    if (fgets(api_key, 256, f_ptr) == NULL) {
        fprintf(stderr, "Error reading API_KEY\n");
        fclose(f_ptr);
        return NULL;
    }

    // Read the stop_id
    char stop_id[256];
    // Error checking
    if (fgets(stop_id, 256, f_ptr) == NULL) {
        fprintf(stderr, "Error reading API_KEY\n");
        fclose(f_ptr);
        return NULL;
    }

    // Close the file to reduce memory
    fclose(f_ptr);

    // Allocate memory to create pointer. 
    ENV* env = (ENV*)malloc(sizeof(ENV));
    if (env == NULL) {
        // Error checking
        return NULL;
    }

    // Remove "API_KEY=" and "STOP_ID="
    char* api_value = strchr(api_key, '=');
    char* stop_value = strchr(stop_id, '=');

    if (api_value == NULL || stop_value == NULL) {
        // Error checking, if cut off too much, return null
        free(env);
        return NULL;
    }

    // Move pointer past the '=' character
    api_value+=2;
    stop_value+=2;

    // Remove newline characters
    api_value[strcspn(api_value, "\r\n")] = '\0';
    stop_value[strcspn(stop_value, "\r\n")] = '\0';

    // Duplicate the string for api_key and stop_id and allocate
    // to the ENV struct
    env->API_KEY = strdup(api_value);
    env->STOP_ID = strdup(stop_value);

    printf("Successfully load environment variables!\n");

    // Return the env. 
    return env;
}

/**
 * Add a callback function that will handle the response
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

/**
 * Return an array of structs. Function takes an input of ENV struct. 
 * 
 * Function create an api request, parse through the information and create instances of structs
 * using a for loop. 
 */
BusInfo_t* api_request(ENV* env) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    // Allocate memory to struct
    BusInfo_t* timetable_array = (BusInfo_t*)malloc(10 * sizeof(BusInfo_t));
    if (timetable_array == NULL) {
        printf("An error occurred while allocating memory to timetable_array\n");
        return NULL;
    }

    curl = curl_easy_init();
    if (curl) {
        char url[512];
        snprintf(url, sizeof(url), 
            "https://api.transport.nsw.gov.au/v1/tp/departure_mon?"
            "outputFormat=rapidJSON"
            "&coordOutputFormat=EPSG:4326"
            "&mode=direct"
            "&type_dm=stop"
            "&name_dm=%s"
            "&departureMonitorMacro=true"
            "&TfNSWDM=true"
            "&version=10.2.1.42",
            env->STOP_ID
        );

        struct curl_slist *headers = NULL;
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), 
            "Authorization: apikey %s", 
            env->API_KEY
        );

        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            free(timetable_array);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return NULL;
        } else {
            // printf("Raw API Response:\n%s\n", chunk.memory);

            cJSON *json = cJSON_Parse(chunk.memory);
            if (json == NULL) {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL) {
                    fprintf(stderr, "Error parsing JSON Response: %s\n", error_ptr);
                }
                free(chunk.memory);
                free(timetable_array);
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return NULL;
            }

            char *json_str = cJSON_Print(json);
            printf("Parsed JSON:\n%s\n", json_str);
            free(json_str);

            cJSON *stopEvents = cJSON_GetObjectItem(json, "stopEvents");
            if (stopEvents == NULL) {
                cJSON *errorMsg = cJSON_GetObjectItem(json, "errorMessage");
                if (errorMsg != NULL && cJSON_IsString(errorMsg)) {
                    fprintf(stderr, "API Error: %s\n", errorMsg->valuestring);
                } else {
                    fprintf(stderr, "No stopEvents found in response\n");
                }
                cJSON_Delete(json);
                free(chunk.memory);
                free(timetable_array);
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return NULL;
            }

            for (int i = 0; i < 10; i++) {
                timetable_array[i].bus_num = NULL;
                timetable_array[i].bus_destination = NULL;
                timetable_array[i].scheduled = NULL;
                timetable_array[i].estimated = NULL;
                timetable_array[i].delay = 0;
            }

            int num_events = cJSON_GetArraySize(stopEvents);
            if (num_events > 10) num_events = 10;

            for (int i = 0; i < num_events; i++) {
                cJSON *event = cJSON_GetArrayItem(stopEvents, i);
                cJSON *transportation = cJSON_GetObjectItem(event, "transportation");
                
                cJSON *number = cJSON_GetObjectItem(transportation, "number");
                timetable_array[i].bus_num = strdup(number->valuestring);

                cJSON *destination = cJSON_GetObjectItem(transportation, "destination");
                cJSON *dest_name = cJSON_GetObjectItem(destination, "name");
                timetable_array[i].bus_destination = strdup(dest_name->valuestring);

                cJSON *planned = cJSON_GetObjectItem(event, "departureTimePlanned");
                timetable_array[i].scheduled = strdup(planned->valuestring);

                cJSON *estimated = cJSON_GetObjectItem(event, "departureTimeEstimated");
                if (estimated) {
                    timetable_array[i].estimated = strdup(estimated->valuestring);
                    
                    char scheduled_time[9];  // HH:MM:SS
                    char estimated_time[9];
                    
                    strncpy(scheduled_time, planned->valuestring + 11, 8);
                    scheduled_time[8] = '\0';
                    strncpy(estimated_time, estimated->valuestring + 11, 8);
                    estimated_time[8] = '\0';

                    int sh, sm, ss, eh, em, es;
                    sscanf(scheduled_time, "%d:%d:%d", &sh, &sm, &ss);
                    sscanf(estimated_time, "%d:%d:%d", &eh, &em, &es);
                    
                    int scheduled_minutes = sh * 60 + sm;
                    int estimated_minutes = eh * 60 + em;
                    timetable_array[i].delay = estimated_minutes - scheduled_minutes;
                    
                    if (timetable_array[i].delay < -720) { // More than 12 hours negative
                        timetable_array[i].delay += 1440;   // Add 24 hours
                    } else if (timetable_array[i].delay > 720) { // More than 12 hours positive
                        timetable_array[i].delay -= 1440;   // Subtract 24 hours
                    }
                }
            }

            cJSON_Delete(json);
        }

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    return timetable_array;
}

void print_timetable(BusInfo_t* timetable) {
    printf("\033[2J\033[H");  // Clear screen
    printf("Current Bus Timetable:\n");
    printf("==========================================\n");
    
    for (int i = 0; i < 10; i++) {
        if (timetable[i].bus_num) {
            printf("Bus %s to %s\n", 
                timetable[i].bus_num, 
                timetable[i].bus_destination);
            printf("Scheduled: %.8s\n", timetable[i].scheduled + 11);  // Show only HH:MM:SS
            if (timetable[i].estimated) {
                printf("Estimated: %.8s (%+d min)\n", 
                    timetable[i].estimated + 11,  // Show only HH:MM:SS
                    timetable[i].delay);
            }
            printf("------------------------------------------\n");
        }
    }

    printf("\nRefreshing after %d seconds", REFRESH_TIME);
}

void free_timetable(BusInfo_t* timetable) {
    for (int i = 0; i < 10; i++) {
        free(timetable[i].bus_num);
        free(timetable[i].bus_destination);
        free(timetable[i].scheduled);
        free(timetable[i].estimated);
    }
    free(timetable);
}

int main() {
    ENV *api_env = loadEnv("../../.env");
    if (!api_env) {
        return 1;
    }

    while (1) {
        BusInfo_t* timetable = api_request(api_env);
        if (timetable) {
            print_timetable(timetable);
            free_timetable(timetable);
        }
        Sleep(REFRESH_TIME * 1000);
    }

    free(api_env);
    return 0;
}