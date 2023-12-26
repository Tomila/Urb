const functions = require('@google-cloud/functions-framework');
const { initializeApp, applicationDefault, cert } = require('firebase-admin/app');
const { getFirestore, Timestamp, FieldValue, Filter } = require('firebase-admin/firestore');

initializeApp({
  projectId: 'goodeffort',
  credential: applicationDefault()
});

const db = getFirestore();
const fetchwindow = 60 //how far from history you want to fetch in mimutes. maximum is 6 months. if you want to use other time measurement refer to aranet docs.


functions.http('helloHttp', (req, res) => {

    let result;
    let data2cloud;
  let apikey = "nicetry"
  //Fetch the list of sensors from api
  fetch('https://aranet.cloud/api/v1/sensors', {
  headers: {
    'application': 'application/json',
    'ApiKey': apikey
  }
})
  .then(response => {
    if (!response.ok) {
      throw new Error('Network response not ok');
    }
    return response.json();
  })
  .then(data => {
    console.log(data.sensors);
    //filter only urbanfarm sensors from the list. this should be filtered with the apikey perms, but that does not appear to be working os this is a workaround
    result = data.sensors.filter(e => e.hasOwnProperty('tags') && e.tags.includes('Urban farm lab'))
    console.log(result);
   
    //This is needed because aranet api cant use sensor ids on payload and they must be in the url
    //the url also cannot contain "," characters and they must be switched to "%2C"
    //then append the duration. if you change the units from minutes to something else change the &minutes to match your choice of units. refer to aranet api docs
    let readingsUrl='';
    result.forEach(e => 
      readingsUrl += e.id + "%2C")
      readingsUrl = readingsUrl.substring(0, readingsUrl.length -3)
      readingsUrl = 'https://aranet.cloud/api/v1/measurements/last?sensor=' + readingsUrl + "&minutes=" + fetchwindow
    console.log("the output is " + readingsUrl)

          //Fetch the actual sensor readings
          fetch(readingsUrl, {
        headers: {
          'application': 'application/json',
          'ApiKey': apikey
        }
      })
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response not ok');
          }
          return response.json();
        })
        .then(data => {
          console.log(data);
          //push data to firestore
          //the finalvalue is used to store both the value and the unit of the sensor reading. you could just post the value but that would be confusing in the data.
          //the units are stored in their own array so you have to match the unit with the unit of the sensor.  
          data.readings.forEach(e=> {
            e.finalvalue = `${e.value} ${data.links.unit[e.unit - 1].name}`
            //this is where the actual posting happens. collection and doc refer to the fields in the firestore. set can be how ever you like or need. alter this to match your needs.
            //for now i just used the time + sensor id as a "tittle" for the entry for ease of reading during testing. this can also be altered in the future. 
          const aTuringRef = db.collection('urban').doc(String(e.time + " Sensor " + e.sensor));
          aTuringRef.set({
            'e.finalvalue': e.finalvalue,
            'sensorID': e.sensor,
            'SensorReadingTime': e.time,
          })}
          )

        })
  })
  .catch(error => {
    // Handle errors here
    console.error('There was a problem with the fetch operation:', error);
  });
  
  //because this is a http function it must retrun something so it jsut returns helllo worlds for now.
  res.send(`Hello ${req.query.name || req.body.name || 'World'}!`);
});
