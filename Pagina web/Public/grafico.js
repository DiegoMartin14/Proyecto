async function fetchData(days) {
  const now = new Date();
  let startDate, endDate;

  // Calcular las fechas según el período seleccionado
  if (days === 1) {
    // Hoy
    startDate = new Date(now.setHours(0, 0, 0, 0)); // Comenzar a medianoche de hoy
    endDate = new Date(now.setHours(23, 59, 59, 999)); // Terminar al final del día
  } else if (days === 2) {
    // Ayer
    startDate = new Date(now.setDate(now.getDate() - 1)); // Comenzar a medianoche del día anterior
    startDate.setHours(0, 0, 0, 0); // Ajustar a medianoche
    endDate = new Date(startDate); // Terminar al final del día anterior
    endDate.setHours(23, 59, 59, 999); // Ajustar al final del día
  } else if (days === 7) {
    // Última semana
    startDate = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000); // 7 días hacia atrás
    endDate = new Date(); // Terminar al momento actual
  } else if (days === 14) {
    // Últimas 2 semanas
    startDate = new Date(now.getTime() - 14 * 24 * 60 * 60 * 1000); // 14 días hacia atrás
    endDate = new Date(); 
  } else if (days === 30) {
    // Último mes
    startDate = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000); // 30 días hacia atrás
    endDate = new Date(); 
  }

  // Convertir a segundos para Firestore
  const startDateEpoch = Math.floor(startDate.getTime() / 1000);
  const endDateEpoch = Math.floor(endDate.getTime() / 1000);

  const dataByTank = { 1: [], 2: [], 3: [], 4: [], 5: [], 6: [] };

  const querySnapshot = await window.db
    .collection("Produccion")
    .where(
      firebase.firestore.FieldPath.documentId(),
      ">=",
      startDateEpoch.toString()
    )
    .where(
      firebase.firestore.FieldPath.documentId(),
      "<=",
      endDateEpoch.toString()
    )
    .orderBy(firebase.firestore.FieldPath.documentId()) // Ordenar por ID (epoch time)
    .get();

  querySnapshot.forEach((doc) => {
    const docData = doc.data();
    const epochTime = parseInt(doc.id); // Convertir el ID a número
    const fechaHoraUTC = new Date(epochTime * 1000); // Convertir epoch time a Date en milisegundos

    const fechaHora = fechaHoraUTC; 

    if (dataByTank[docData.Tanque]) {
      dataByTank[docData.Tanque].push({
        x: fechaHora, // fecha para el eje X
        y: parseFloat(docData.temperatura), // Convierte la temperatura a número para el eje Y
      });
    }
  });

  return dataByTank;
}

// Configuración inicial del gráfico
const ctx = document.getElementById("temperatureChart").getContext("2d");
let temperatureChart = new Chart(ctx, {
  type: "line",
  data: {
    datasets: [
      { label: "Tanque 1", data: [], borderColor: "red", fill: false },
      { label: "Tanque 2", data: [], borderColor: "blue", fill: false },
      { label: "Tanque 3", data: [], borderColor: "green", fill: false },
      { label: "Tanque 4", data: [], borderColor: "orange", fill: false },
      { label: "Tanque 5", data: [], borderColor: "purple", fill: false },
      { label: "Tanque 6", data: [], borderColor: "cyan", fill: false },
    ],
  },
  options: {
    scales: {
      x: {
        type: "time",
        time: {
          unit: "day", 
          tooltipFormat: "PP", 
          displayFormats: {
            day: "MMM d", 
            hour: "HH:mm", 
          },
        },
        title: {
          display: true,
          text: "Fecha",
        },
        bounds: "data", 
      },
      y: {
        title: {
          display: true,
          text: "Temperatura (°C)",
        },
        beginAtZero: true,
      },
    },
    plugins: {
      tooltip: {
        callbacks: {
          title: function (tooltipItems) {
            const date = tooltipItems[0].label;
            return [`${date}`]; 
          },
          label: function (tooltipItem) {
            const datasetLabel = tooltipItem.dataset.label || "";
            const temperature = tooltipItem.raw.y.toFixed(1) + " °C";
            const time =
              new Date(tooltipItem.raw.x).toISOString().substr(11, 5) + " hs";
            return [`${datasetLabel}: ${temperature}`, time];
          },
        },
      },
    },
  },
});

// Función para actualizar el gráfico con nuevos datos
async function updateChart(days) {
  try {
    const dataByTank = await fetchData(days);

    temperatureChart.data.datasets.forEach((dataset, index) => {
      dataset.data = dataByTank[index + 1] || []; 
    });

    temperatureChart.update(); // Actualizar el gráfico con los nuevos datos
  } catch (error) {
    console.error("Error al actualizar el gráfico:", error);
  }
}

// Inicializar el gráfico con el lapso de tiempo seleccionado
async function initializeChart() {
  const timeRange = document.getElementById("timeRange").value;
  await updateChart(parseInt(timeRange));

  document
    .getElementById("timeRange")
    .addEventListener("change", async (event) => {
      const selectedDays = parseInt(event.target.value);
      await updateChart(selectedDays);
    });
}
