import React, { useEffect, useState } from 'react';
import { ResponsiveContainer, LineChart, CartesianGrid, XAxis, YAxis, Tooltip, Legend, Line } from 'recharts';
import './App.css'; // CSS per la grafica
import { io } from 'socket.io-client';



// approccio bottom up, componente per componente (per rendere più facile la modifica su css e l'aggiunta di componenti nuovi)
// Componente per una singola riga della tabella

function TableRow({ item }) { {/*corpo tabella*/}
  const alarmClass = item.allarme ? 'allarme-true' : 'allarme-false'; // Classe CSS diversa se c'è allarme

  return (              
    <tr>    {/*riga celle dati*/}
      <td>{item.ID}</td>
      <td>{item.timestamp}</td>
      <td>{item.accel_x}</td>
      <td>{item.accel_y}</td>
      <td>{item.accel_z}</td>
      <td>{item.gyro_x}</td>
      <td>{item.gyro_y}</td>
      <td>{item.gyro_z}</td>
      <td>{item.roll}</td>
      <td>{item.pitch}</td>
      <td>{item.yaw}</td>
      <td className= {item.allarme ? 'allarme-true' : 'allarme-false'}>
        {item.allarme ? 'ALLARME' : 'OK'}</td>
    </tr>
  );
}


// Componente che comprende tutta la tabella

function DataTable({ data }) {
  if (!data || data.length === 0) {
    return <p>Nessun dato da visualizzare</p>; // Messaggio se non ci sono dati
  }

  return (
    <table border="1">
      <thead>      {/*intestazione tabella*/}
        <tr> {/*riga intestazione*/}
          <th>ID</th>  {/*celle di intestazione*/}
          <th>Timestamp</th>
          <th>Accel X</th>
          <th>Accel Y</th>
          <th>Accel Z</th>
          <th>Gyro X</th>
          <th>Gyro Y</th>
          <th>Gyro Z</th>
          <th>Roll</th>
          <th>Pitch</th>
          <th>Yaw</th>
          <th>Allarme</th>
        </tr>
      </thead>
      <tbody>
        {data.map((item, index) => (
          <TableRow key={index} item={item} /> // Uso della funzione più semplice
        ))}
      </tbody>
    </table>
  );
}

//funzione grafico

function SensorChart({ data}){
  if(!data || data.lenght === 0){
    return <p>Nessun dato da visualizzare</p>;
  }

  return (
    <ResponsiveContainer width='100%' height={500}>
      <LineChart data= {data} >
        <CartesianGrid strokeDasharray="5 3"/>
          <XAxis dataKey="timestamp"/>
          <YAxis/>
          <Tooltip/>
          <Legend/>
          <Line type="monotone" dataKey="roll" stroke="#003fc7" name="Roll" strokeWidth="3" />
          <Line type="monotone" dataKey="pitch" stroke="#0ebd34" name="Pitch" strokeWidth="3" />
          <Line type="monotone" dataKey="yaw" stroke="#ff8800" name="Yaw" strokeWidth="3" />
      </LineChart>
    </ResponsiveContainer>
  );
}

//componente barra di ricerca

function SearchBar({ searchId, onSearchIdChange}) {
  return (
    <input
      type="text"
      placeholder="Cerca per ID"
      value={searchId}
      onChange={(e) => onSearchIdChange(e.target.value)}
      className="search-bar"
    />
  );
}


// Componente principale che unisce tutto

function App() {
  const [data, setData] = useState([]); // Stato per dati
  const [searchId, setSearchId] = useState('');

  // Caricamento dati iniziali + socket.io
  useEffect(() => {
    const socket = io('http://insertyourdomain:3001',{
      //secure: true,
      //rejectUnauthorized: false
    }); // collegamento socket.io
  

    // Funzione per caricare dati da backend
    const fetchData = async () => {  
      try {
        const response = await fetch('http://insertyourdomain/data');
        const jsonData = await response.json();
        setData(jsonData);
      } catch (error) {
        console.error('Errore nel caricamento dei dati:', error);
      }
    };

    // Carica dati al primo caricamento
    fetchData();

    // Ricarica la pagina web quando avviene dataUpdated
    socket.on('dataUpdated', (newData) => {
      console.log('Nuovi dati ricevuti via socket:', newData);
      fetchData(); // Ricarica l'intero set di dati
    });

    // Cleanup: chiusura socket se il componente viene smontato
    return () => socket.disconnect();
  }, []);

  // Filtro dati
  const filteredData = data.filter((item) =>
    searchId === '' || item.ID.toString().includes(searchId)
  );

  //Filtro dati per grafico
  const dataForChart = filteredData
  .filter((item) => item.ID.toString() === searchId) //filtra i dati per id
  .sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp)); //ordina in base al timestamp

  return (
    <div>
      <h1>Sensor Data</h1>
      <SearchBar searchId={searchId} onSearchIdChange={setSearchId} /> {/*implementa funzione barra ricerca*/}
      <DataTable data={filteredData} /> {/*implementa funzione tabella*/}
      <h2>Graphs for IDs</h2>
      <SensorChart data={dataForChart}/>
    </div>
  );
}

export default App;
