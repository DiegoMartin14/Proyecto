// Función para guardar datos en Firestore
async function guardar() {
    const tanqueSeleccionado = document.getElementById("tank-select").value;
    const nombre = document.getElementById("tipoCerveza").value;
    const tempMax = document.getElementById("maxTemp").value;
    const tempMin = document.getElementById("minTemp").value;
  
    try {
      await db
        .collection("Configuracion")
        .doc(`Tanque${tanqueSeleccionado}`)
        .set({
          Nombre: nombre,
          TempMax: tempMax,
          TempMin: tempMin,
        });
      console.log(
        `Documento para Tanque ${tanqueSeleccionado} escrito exitosamente!`
      );
      document.getElementById("tipoCerveza").value = "";
      document.getElementById("minTemp").value = "";
      document.getElementById("maxTemp").value = "";
    } catch (error) {
      console.error("Error al escribir el documento: ", error);
    }
  }
  
  window.guardar = guardar;