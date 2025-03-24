import express from 'express'; //libreria per la gesitione delle richieste
import fs from 'fs'; //libreria per la gestione dei file (salvare e leggere i json)
import cors from 'cors'; //libreria per rispondere alle richieste da react (da fare)
import http from 'http';
import https from 'https'; // libreria per creare il server
import { Server } from  'socket.io'; //libreria per la gestione delle notifiche tra backend e frontend



const app = express();
const server = http.createServer(app); // crea il server http

/*const options = {  //legge i file del certificato e della chiave SSL
  key: fs.readFileSync('/home/matteo/develop/httpsrv/localhost-key.pem'),
  cert: fs.readFileSync('/home/matteo/develop/httpsrv/localhost.pem'),
};*/

//const server = https.createServer(options,app); //crea server https

const io = new Server(server, { //crea il server socket.io
  cors: {
    origin: '*' //permette l'accesso da qualsiasi origine
  }
});

app.use(express.json()); //permette di leggere i json
app.use(cors({  //permette di gestire le richieste da parte di react
  origin: '*',
  //methods: ['GET', 'POST'],
  //allowedHeaders: ['Content-Type']
})); 
const port = 3001;
const dataFile = '/home/matteo/develop/httpsrv/receivedData.json';

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


function saveData(data){ //salva i dati ricevuti
  fs.writeFileSync(dataFile, JSON.stringify(data, null, 2)); //scrive i dati in un file di testo
}


//handler richiesta GET
app.get('/data', (req, res) => {
  const existingData= loadData(); //carica i dati già presenti
  res.json(existingData); //risponde con i dati caricati
});

// GET handler per la pagina di benvenuto
app.get('/', (req, res) => {
  res.send('<h1>Welcome to my server!</h1><p>Vai su <a href="/data">/data</a> per vedere i dati.</p>');
});

// POST request handler
app.post('/data', (req, res) => {  
    const newData = req.body; //riceve i dati inviati
    console.log('Dati ricevuti: ', newData); //stampa le coppie chiave valore ricevute

    let existingData = loadData(); //carica i dati già presenti
    existingData.push(newData); //aggiunge i nuovi dati a quelli già presenti
    saveData(existingData); //salva i dati aggiornati

    io.emit('dataUpdated', newData); //manda la notifica a react che ci sono nuovi dati

    res.status(200).send( 'Dati ricevuti e salvati correttamente.');
  });
  
//gestisce la connessione socket.io
io.on('connection', (socket) => {
  console.log('Client connesso:', socket.id);

  socket.on('disconnect', () => {
    console.log('Client disconnesso:', socket.id);
  });
});

//avvio il server (sostituisce la vecchia app.listen)
server.listen(port, () => {
  console.log(`Il server è in ascolto sulla porta: ${port}`);
});