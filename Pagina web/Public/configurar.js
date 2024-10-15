
// Función para cargar configuración del tanque seleccionado
async function mostrarConfiguracion() {
    const tankId = document.getElementById("tank-select").value;
  
    if (tankId) {
      try {
        // Referencia al documento en Firestore
        const docRef = firebase
          .firestore()
          .collection("Configuracion")
          .doc(`Tanque${tankId}`);
        const doc = await docRef.get();
  
        if (doc.exists) {
          // Obtener los datos del documento
          const data = doc.data();
          console.log("Datos del documento:", data); // muestra los datos por consola
  
          // Mostrar los datos en la sección de configuración
          document.querySelector("#configTipoCerveza span").textContent =
            data.Nombre || "N/A";
          document.querySelector("#configMinTemp span").textContent =
            data.TempMin || "N/A";
          document.querySelector("#configMaxTemp span").textContent =
            data.TempMax || "N/A";
        } else {
          console.log(
            "No se encontró la configuración para el tanque seleccionado."
          );
        }
      } catch (error) {
        console.error("Error al obtener la configuración:", error);
      }
    } else {
      alert("Por favor, selecciona un tanque.");
    }
  }