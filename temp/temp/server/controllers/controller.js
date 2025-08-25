import mqtt from "mqtt"

const protocol = 'tcp';
const host = 'broker.mqtt.cool';
const port = '1883';
const clientId = `mqtt_${Math.random().toString(16).slice(3)}`;
const connectUrl = `${protocol}://${host}:${port}`;

let latestMsg = null;

const client = mqtt.connect(connectUrl, {
    clientId,
    clean: true,
    connectTimeout: 4000,
    reconnectPeriod: 1000,
});

client.on('connect', () => {
    console.log('Connected');
    client.subscribe(["/dht/aryan"], () => {
        console.log('Subscribed to /dht/aryan');
    });
});

client.on('message', (topic, payload) => {
    console.log('Received Message:', topic, payload.toString());
    latestMsg = payload.toString();
});

export function getTempMoisture(req, res) {
    res.send(latestMsg || "No data received yet");
}
