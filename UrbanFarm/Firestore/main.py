from datetime import datetime, timedelta, timezone
from google.cloud import firestore, storage

# Replace with your Firebase Project ID inside project=''
client = firestore.Client(project='')

# Replace with your preferred Google Cloud Storage bucket name
bucket_name = "YOUR_BUCKET_HERE"

# Replace with your preferred folder/filename.csv inside Google Cloud Storage
base_blob_name = "data/sensor_data"

# Replace with the frequency of .csv file creation per minutes to Google Cloud Bucket, currently creates one every 24 * 60 minutes
csv_frequency_created = 24 * 60

# How often we create a new document file inside Firestore?
CREATE_DOCUMENT_INTERVAL = timedelta(minutes=0.25)

# Attempting to sync Cloud Functions with Firestore here...
MIN_PRINT_INTERVAL = timedelta(minutes=0.15)

storage_client = storage.Client()
bucket = storage_client.bucket(bucket_name)

sensor_ids = set()
last_creation_times = {}
last_printed_times = {}

# Does Firebase collection exist? To avoid multiple of same collection
def collection_exists(collection_name):
    collection_ref = client.collection(collection_name)
    query = collection_ref.limit(1)
    result = next(query.stream(), None)
    return bool(result) and result.id == 'check_document'

# Check if Firestore document exists to avoid multiple instances
def document_exists(collection_name, doc_id):
    doc_ref = client.collection(collection_name).document(doc_id)
    return doc_ref.get().exists

# Create a unique collection and doc based on sensor_id, kampus, huone if it does not already exist
def create_unique_collection_and_doc_id(sensor_id, kampus, huone):
    kampus_collection_name = f'kampus_{kampus.replace(" ", "_").lower()}' if kampus else 'default_kampus'
    huone_collection_name = f'{huone.replace(" ", "_").lower()}' if huone else 'default_huone'
    doc_id = f"{sensor_id}_{datetime.now().strftime('%Y-%m-%d-%H:%M:%S')}"


    print(f'kampus_collection_name: {kampus_collection_name}')
    print(f'huone_collection_name: {huone_collection_name}')

    if not collection_exists(kampus_collection_name):
        client.collection(kampus_collection_name).document('check_document').set({})

    huone_collection_ref = client.collection(kampus_collection_name).document('check_document').collection(huone_collection_name)

    if not collection_exists(huone_collection_ref.id):
        huone_collection_ref.document('check_document').set({})

    current_time = datetime.now()
    current_time += timedelta(hours=2)

    if sensor_id in last_printed_times:
        last_printed_time = last_printed_times[sensor_id]
        if (current_time - last_printed_time) >= MIN_PRINT_INTERVAL:
            print(f'New document created for sensor_id: {sensor_id}')
            last_printed_times[sensor_id] = current_time
    else:
        print(f'New document created for sensor_id: {sensor_id}')
        last_printed_times[sensor_id] = current_time

    return kampus_collection_name, huone_collection_name, doc_id

# Update data based on incoming data
def update_sensor_data(sensor_id, temperature, humidity, moisture, kampus=None, huone=None):
    if kampus is None or huone is None:
        # Handle the case where kampus or huone is missing
        kampus = 'default_kampus'
        huone = 'default_huone'

    kampus_collection_name, huone_collection_name, doc_id = create_unique_collection_and_doc_id(sensor_id, kampus, huone)
    doc_ref = client.collection(kampus_collection_name).document(huone_collection_name).collection('sensor_data_' + sensor_id).document(doc_id)

    sensor_data = {
        'sensor': sensor_id,
        'last_data_time': datetime.now(),
    }

    # Add temperature, humidity, and moisture if available
    if temperature is not None:
        sensor_data['temperature'] = temperature
    if humidity is not None:
        sensor_data['humidity'] = humidity
    if moisture is not None:
        sensor_data['moisture'] = moisture

    doc_ref.set(sensor_data)
    print(f"Adding data to Firestore: {kampus_collection_name}/{huone_collection_name}/sensor_data -> {doc_id}")

    # Now, update Google Cloud Storage as before
    print("Adding data to Google Cloud Storage", bucket_name, "->", base_blob_name)
    write_sensor_data_to_gcs(bucket_name, base_blob_name, sensor_data)

# Attempting to sync document creation with google cloud functions here...
def create_new_document_needed(sensor_id):
    current_time = datetime.now()

    if sensor_id not in last_creation_times:
        print(f'Creating new document for sensor_id: {sensor_id} at {current_time}')
        last_creation_times[sensor_id] = current_time
        return True
    else:
        last_creation_time = last_creation_times[sensor_id]
        time_since_last_creation = current_time - last_creation_time
        print(f'Time since last creation for sensor_id {sensor_id}: {time_since_last_creation}')
        
        if time_since_last_creation >= CREATE_DOCUMENT_INTERVAL:
            new_creation_time = current_time
            print(f'Creating new document for sensor_id: {sensor_id} at {new_creation_time}')
            last_creation_times[sensor_id] = new_creation_time
            return True
        else:
            print(f'Not creating new document for sensor_id: {sensor_id}')
            return False

# Create the CSV file in Google Cloud Storage
def get_blob_name(base_blob_name):
    # Replace 'YOUR_TIMEZONE' with the actual time zone offset (e.g., 'UTC+2')
    time_zone_offset = '+02:00'
    
    current_time = datetime.now(timezone(timedelta(hours=int(time_zone_offset[:3]), minutes=int(time_zone_offset[4:]))))
    rounded_time = current_time - timedelta(minutes=current_time.minute % csv_frequency_created,
                                             seconds=current_time.second,
                                             microseconds=current_time.microsecond)
    formatted_time = rounded_time.strftime("%Y-%m-%d_%H:%M:%S")
    return f"{base_blob_name}/{formatted_time}.csv"

# Write sensor data to the CSV file
def write_sensor_data_to_gcs(bucket_name, base_blob_name, sensor_data):
    blob_name = get_blob_name(base_blob_name)
    blob = storage_client.bucket(bucket_name).blob(blob_name)

    # Read existing content if the blob exists
    existing_content = blob.download_as_text() if blob.exists() else ""

    # Define individual headers
    headers = ["sensor", "temperature", "humidity", "moisture", "last_data_time"]

    # Create a new row with sensor data
    new_row = [
        sensor_data.get('sensor', ''),
        sensor_data.get('temperature', ''),
        sensor_data.get('humidity', ''),
        sensor_data.get('moisture', ''),
        sensor_data.get('last_data_time', '').strftime('%Y-%m-%d %H:%M:%S')
    ]

    # If there's no existing content, create a new CSV with headers and the new row
    if not existing_content:
        updated_content = '\n'.join([','.join(headers), ','.join(new_row)])
    else:
        # Otherwise, combine the existing rows with the new row
        existing_rows = [row for row in existing_content.split('\n') if row]
        updated_content = '\n'.join(existing_rows + [','.join(new_row)])

    # Write the data to the CSV file
    blob.upload_from_string(updated_content, content_type='text/csv')

    print(f"Updated CSV file in Google Cloud Storage: {bucket_name} -> {blob_name}")

# The main pubsub to firestore function
def pubsub_fire(event, context):
    import base64

    print(f'This function was triggered by messageId {context.event_id}, published at {context.timestamp} to {context.resource["name"]}!')

    message = ''

    if 'data' in event:
        try:
            message = base64.b64decode(event['data']).decode('utf-8')
        except UnicodeDecodeError:
            binary_data = base64.b64decode(event['data'])
            message = None

    if message is not None:
        print(f'message: {message}')

    temperature = event['attributes'].get('temperature')
    humidity = event['attributes'].get('humidity')
    moisture = event['attributes'].get('moisture')
    kampus = event['attributes'].get('kampus', 'default_kampus')
    huone = event['attributes'].get('huone', 'default_huone')

    sensor_id = event['data']
    sensor_ids.add(sensor_id)
    print(f'Received sensor_id: {sensor_ids}')

    if temperature is not None:
        if create_new_document_needed(sensor_id):
            last_creation_times[sensor_id] = datetime.now()
        update_sensor_data(sensor_id, temperature, humidity, moisture, kampus, huone)
