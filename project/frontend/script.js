// Create WebSocket connection.
const socket = new WebSocket('ws://localhost:8001');
const c = document.getElementById("graph");
const ctx = c.getContext("2d");
const width = c.width;
const height = c.height
const scale = 40

var backlog = [];

// Connection opened
socket.addEventListener('open', function (event) {
	console.log("Socket is open!");
});

// Listen for messages
socket.addEventListener('message', function (event) {
	const data = JSON.parse(event.data);
	updateCanvas(data)
});

function clear() {
	ctx.clearRect(0, 0, c.width, c.height);
}

function updateCanvas(data) {
	backlog.push(data);
	clear();

	for(var i = 1; i < backlog.length; i++) {
		const prev = backlog[i - 1];
		const curr = backlog[i];
		ctx.moveTo((i - 1) * scale, prev.cpu * scale);
		ctx.lineTo(i * scale, curr.cpu * scale);
		ctx.stroke();
	}
	
	if(backlog.length > width / scale) {
		console.log(backlog);
		backlog.shift();
		console.log(backlog);
	}
}
