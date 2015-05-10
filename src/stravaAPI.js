var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function getStats() {
  // Construct URL
  var athlete = "3237232";
  var url = "http://crin.co.uk/stravaWatchface/getStats.php?a=" + athlete;

  // Send request to crin.co.uk
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with stat info
      var json = JSON.parse(responseText);

      // Monthly miles
      var monthMiles = json.mM;
					console.log("Monthly total miles is: " + monthMiles);

      // Monthly Elevation
      var monthElevation = json.mE;      
      console.log("Monthly total elevation gain is: " + monthElevation);

      // Total Miles
      var totalMiles = json.tM;      
      console.log("All-time total miles is: " + totalMiles);

      // Total Elevation
      var totalElevation = json.tE;      
      console.log("All-time total elevation gain is: " + totalElevation);
      
      // Assemble dictionary using our keys
      var dictionary = {
       "KEY_MONTHMILES": monthMiles,
       "KEY_MONTHELEVATION": monthElevation,
       "KEY_TOTALMILES": totalMiles,
							"KEY_TOTALELEVATION": totalElevation
						};

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Stats info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending stats info to Pebble!");
        }
      );
    }      
  );
}



// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getStats();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getStats();
  }                     
);