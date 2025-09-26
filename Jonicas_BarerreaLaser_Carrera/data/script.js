var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// Variables de estado
var raceState = "WAITING"; // WAITING, RUNNING, FINISHED
var startTime = 0;
var timerInterval = null;

// Inicializar WebSocket cuando la página carga
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
    updateUI();
}

function initWebSocket() {
    console.log('Intentando abrir conexión WebSocket...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
    websocket.onerror = onError;
}

function onOpen(event) {
    console.log('Conexión WebSocket establecida');
    updateConnectionStatus(true);
    // Solicitar estado inicial
    sendMessage("getStatus");
}

function onClose(event) {
    console.log('Conexión WebSocket cerrada');
    updateConnectionStatus(false);
    // Intentar reconectar después de 3 segundos
    setTimeout(initWebSocket, 3000);
}

function onError(event) {
    console.error('Error en WebSocket:', event);
    updateConnectionStatus(false);
}

function onMessage(event) {
    console.log('Mensaje recibido:', event.data);
    try {
        var data = JSON.parse(event.data);

        if (data.raceState) {
            raceState = data.raceState;
            updateRaceStatus(raceState);

            if (data.raceState === "SEMAPHORE" && data.light) {
                const statusElement = document.getElementById('raceStatus');
                if (data.light === "RED") statusElement.textContent = "SEMAFORO ROJO";
                if (data.light === "YELLOW") statusElement.textContent = "SEMAFORO AMARILLO";
                if (data.light === "GREEN") statusElement.textContent = "SEMAFORO VERDE";
            }
        }

        if (data.raceTime !== undefined) updateTimer(data.raceTime);

    } catch (e) {
        console.error('Error al parsear mensaje:', e);
    }
}

function sendMessage(message) {
    if (websocket.readyState === WebSocket.OPEN) {
        websocket.send(message);
        console.log('Mensaje enviado:', message);
    } else {
        console.error('WebSocket no está conectado');
    }
}

// Funciones de control de la carrera
function startRace() {
    if (raceState === "WAITING") {
        sendMessage("startRace");
        console.log('Iniciando carrera...');
    }
}

function resetRace() {
    sendMessage("resetRace");
    console.log('Reiniciando sistema...');
}

// Funciones de actualización de UI
function updateRaceStatus(status) {
    const statusElement = document.getElementById('raceStatus');
    const statusCard = statusElement.parentElement;
    const timerSection = document.getElementById('timerSection');
    const startBtn = document.getElementById('startBtn');

    statusCard.classList.remove('status-waiting', 'status-running', 'status-finished', 'status-error');

    switch (status) {
        case "WAITING":
            statusElement.textContent = "ESPERANDO";
            statusCard.classList.add('status-waiting');
            timerSection.style.display = "none";
            startBtn.disabled = false;
            startBtn.textContent = "INICIAR CARRERA";
            stopTimer();
            break;

        case "SEMAPHORE":
            statusElement.textContent = "SEMAFORO EN CURSO";
            statusCard.classList.add('status-waiting');
            timerSection.style.display = "none";
            startBtn.disabled = true;
            startBtn.textContent = "EN SEMAFORO...";
            stopTimer();
            break;

        case "RUNNING":
            statusElement.textContent = "CARRERA EN CURSO";
            statusCard.classList.add('status-running');
            timerSection.style.display = "block";
            startBtn.disabled = true;
            startBtn.textContent = "CARRERA INICIADA";
            break;

        case "FINISHED":
            statusElement.textContent = "CARRERA FINALIZADA";
            statusCard.classList.add('status-finished');
            timerSection.style.display = "block";
            startBtn.disabled = true;
            startBtn.textContent = "FINALIZADA";
            stopTimer();
            break;

        case "FALSE_START":
            statusElement.textContent = "LARGADA FALSA";
            statusCard.classList.add('status-error');
            timerSection.style.display = "none";
            startBtn.disabled = true;
            startBtn.textContent = "RESET NECESARIO";
            //TODO: no se muestra bien el cartel de falsa largada, se va rapidamente

            // Bloquea el reset por 5 segundos
            const resetBtn = document.getElementById('resetBtn');
            resetBtn.disabled = true;
            setTimeout(() => { resetBtn.disabled = false; }, 5000);

            stopTimer();
            break;

        default:
            statusElement.textContent = "ERROR";
            statusCard.classList.add('status-error');
            timerSection.style.display = "none";
            startBtn.disabled = false;
            startBtn.textContent = "REINTENTAR";
            stopTimer();
    }
}

function updateTimer(timeMs) {
    const timerElement = document.getElementById('raceTimer');
    const formattedTime = formatTime(timeMs);
    timerElement.textContent = formattedTime;
}//TODO: se muetra solo en segundos

function formatTime(timeMs) {
    const minutes = Math.floor(timeMs / 60000);
    const seconds = Math.floor((timeMs % 60000) / 1000);
    const milliseconds = timeMs % 1000;
    
    return `${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}.${milliseconds.toString().padStart(3, '0')}`;
}

function stopTimer() {
    if (timerInterval) {
        clearInterval(timerInterval);
        timerInterval = null;
    }
}

function updateConnectionStatus(connected) {
    const connectionElement = document.getElementById('connectionStatus');
    connectionElement.classList.remove('connected', 'disconnected');
    
    if (connected) {
        connectionElement.textContent = "Conectado";
        connectionElement.classList.add('connected');
    } else {
        connectionElement.textContent = "Desconectado";
        connectionElement.classList.add('disconnected');
    }
}

// function updateLastUpdate() {
//     const now = new Date();
//     const timeString = now.toLocaleTimeString();
//     document.getElementById('lastUpdate').textContent = timeString;
// }

function showError(errorMessage) {
    // Podrías implementar una notificación de error más sofisticada aquí
    console.error('Error del sistema:', errorMessage);
    alert('Error: ' + errorMessage);
}

function updateUI() {
    // Inicializar la UI con valores por defecto
    updateRaceStatus("WAITING");
    updateConnectionStatus(false);
    document.getElementById('lastUpdate').textContent = "--:--:--";
    document.getElementById('raceTimer').textContent = "00:00.000";
}