import express from 'express'; // Libreria per la gestione delle richieste HTTP
import fs from 'fs'; // Libreria per leggere e scrivere file JSON
import path from 'path'; // Libreria per gestire i percorsi dei file
import http from 'http'; // Libreria per creare un server HTTP
import https from 'https';
import { Server } from 'socket.io'; // Libreria per WebSocket
import { fileURLToPath } from 'url'; //libreria per usare path

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const options= {
  key: fs.readFileSync('/home/matteo/develop/server2/server-key.pem'),
  cert: fs.readFileSync('/home/matteo/develop/server2/server-cert.pem'),
};

const app = express();
//const server = http.createServer(app); // Crea il server HTTP
const server = https.createServer(options, app); //Crea server HTTPS 
const io = new Server(server, { // Crea il server WebSocket
  cors: {
    origin: '*'
  }
});

const port = 3001;
const dataFile = path.join(__dirname, 'newReading.json'); // Percorso del file JSON

// Chiamate per gestire i file json e per servire i file per il frontend.
app.use(express.json()); 
app.use(express.static('public')); // Serve i file nella cartella public , quindi html e grafica.css

function loadData(){ //carica i dati già salvati se esistono
  if(fs.existsSync(dataFile)){ //controlla se il file esiste
    const rawData = fs.readFileSync(dataFile, 'utf-8'); //se il file esiste lo legge in maniera sincrona
    if (rawData.trim() === '') {
      return []; // Se il file è vuoto, ritorno array vuoto
    }
    try {
      // Provo a fare il parse del JSON
      return JSON.parse(rawData);
    } catch (err) {
      console.error('Errore nel parsing del file JSON:', err);
      return []; // Se il JSON è corrotto, ritorno array vuoto
  }
}
  return []; //se il file non esiste ritorna un array vuoto
}

// Funzione per salvare i dati nel file JSON
function saveData(data) {
    fs.writeFileSync(dataFile, JSON.stringify(data, null, 2)); //salva i dati ricevuti in un file in locale
}

// handler get alla route /data
app.get('/data', (req, res) => {
    const existingData = loadData(); //carica i dati già presenti
    res.json(existingData); //risponde con il json contenete i dati
});

// handler richiesta get pagina princincipale
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html')); //risponde con la pagina html
});

//handler post 
app.post('/data', (req, res) => {  
    const newData = req.body; 
    console.log('Dati ricevuti:', newData); //stampa nel console log le coppie chiave valore ricevute

    let existingData = loadData(); //carica i dati già presenti
    existingData.push(newData); //aggiunge i nuovi dati a quelli già presenti
    saveData(existingData); //salva i dati aggiornati

    io.emit('dataUpdated', newData); // Manda i nuovi dati ai client connessi via WebSocket

    res.status(200).send('Dati ricevuti e salvati correttamente.');
});

// Gestisce le connessioni WebSocket
io.on('connection', (socket) => {
    console.log('Client connesso:', socket.id);

    // Quando un client si connette, gli inviamo i dati attuali
    socket.emit('initData', loadData());

    socket.on('disconnect', () => {
        console.log('Client disconnesso:', socket.id);
    });
});

// avvio server http tramite websocket.io
server.listen(port, () => {
    console.log(`Server in ascolto sulla porta: ${port}`);
});