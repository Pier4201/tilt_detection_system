import express from 'express'; // library to handle HTTP requests
import fs from 'fs'; // library to read and write on JSON files
import path from 'path'; // librry to handle file paths
import http from 'http'; // HTTP library to create http server (when in use)
import https from 'https'; //HTTPS library to create https server (when in use)
import { Server } from 'socket.io'; // WebSocket library
import { fileURLToPath } from 'url'; // path library

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const options= {         //adds certificates for tsl
  key: fs.readFileSync('/home/matteo/develop/server/server-key.pem'),
  cert: fs.readFileSync('/home/matteo/develop/server/server-cert.pem'),
};

const app = express();
//const server = http.createServer(app); // Creates HTTP server (if u want to set up http server just comment the next line and uncomment this one)
const server = https.createServer(options, app); //Creates HTTPS server
const io = new Server(server, { // creates websocket communication between backend server and interactive html page
  cors: {
    origin: '*'
  }
});

const port = 3001;
const dataFile = path.join(__dirname, 'newReading.json'); // file path to JSON file where data is read and stored

// calls to handle json file and  to serve it to the front end
app.use(express.json()); 
app.use(express.static('public')); // serves ui files in public dir: index.html and grafica.css

function loadData(){ //loads data if already exists
  if(fs.existsSync(dataFile)){ //checks if data exists
    const rawData = fs.readFileSync(dataFile, 'utf-8'); //if exists its read in sync in utf-8n format
    if (rawData.trim() === '') {
      return []; // if empty it returns and empty array
    }
    try {
      // tryes to parse JSON file
      return JSON.parse(rawData);
    } catch (err) {
      console.error('Error in parsing file JSON:', err);
      return []; //returns empty array is the JSON is corrupt
  }
}
  return []; //if the file doesn't exit returns an empty array
}

// function to save data into our JSON
function saveData(data) {
    fs.writeFileSync(dataFile, JSON.stringify(data, null, 2)); //saves data in a file stored locally
}

// handler get to /data route
app.get('/data', (req, res) => {
    const existingData = loadData(); //loads existing data
    res.json(existingData); //responds with JSON file
});

// handler get request to homepage
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html')); //responds with html page
});

//handler post 
app.post('/data', (req, res) => {  
    const newData = req.body; 
    console.log('Received Data:', newData); //prints received key value couples from JSON

    let existingData = loadData(); //loads existing data
    existingData.push(newData); //adds new data to file
    saveData(existingData); //saves updated data

    io.emit('dataUpdated', newData); //Sends updated data to connected clients via WebSocket

    res.status(200).send('Data successfully received and saved');
});

// Handles WebSocket connection
io.on('connection', (socket) => {
    console.log('Client connected:', socket.id);

    // Sends current data to client upon connecting
    socket.emit('initData', loadData());

    socket.on('disconnect', () => {
        console.log('Client disconnected:', socket.id);
    });
});

// starts server via websocket
server.listen(port, () => {
    console.log(`Server in ascolto sulla porta: ${port}`);
});