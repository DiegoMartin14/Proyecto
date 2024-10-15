// Llama a la función `initializeFirebase` cuando el documento esté cargado
document.addEventListener("DOMContentLoaded", initializeFirebase);

// Inicializar Firebase y definir `db` globalmente
async function initializeFirebase() {
  const firebaseConfig = {
    apiKey: "AIzaSyCqgQk4Xdb40sz2F0qcRfAYGUAYbE10dQ8",
    authDomain: "colinas-4cfb3.firebaseapp.com",
    projectId: "colinas-4cfb3",
    storageBucket: "colinas-4cfb3.appspot.com",
    messagingSenderId: "816605004914",
    appId: "1:816605004914:android:7ac1a6998dcc415f30aeeb",
  };

  // Inicializar Firebase
  firebase.initializeApp(firebaseConfig);
  const db = firebase.firestore();
  const auth = firebase.auth();

  // Exponer `db` globalmente
  window.db = db;
  window.auth = auth;

  console.log("Conexión a Firebase establecida correctamente.");

  // Autenticación Anónima
  try {
    await auth.signInAnonymously();
    console.log("Usuario autenticado anónimamente.");
  } catch (error) {
    console.error("Error de autenticación:", error);
  }

  if (firebase.auth().currentUser) {
    console.log("Usuario autenticado:", firebase.auth().currentUser.uid);
  } else {
    console.log("No hay usuario autenticado.");
  }

  // Inicializar gráfico o cualquier otra función necesaria
  initializeChart();
}
