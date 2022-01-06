// Create WebSocket connection.
const socket = new WebSocket('ws://localhost:8001');

var canvases = {
	cpu: document.getElementById("cpu-graph"),
	mem: document.getElementById("mem-graph"),
	rtt: document.getElementById("rtt-graph"),
};
var ctx = {
	cpu: canvases.cpu.getContext("2d"),
	mem: canvases.mem.getContext("2d"),
	rtt: canvases.rtt.getContext("2d"),
};

const scale = 2
const width = canvases.cpu.width;
const height = canvases.cpu.height
const maxBacklog = canvases.cpu.width / scale;

ctx.cpu.lineWidth = scale;
ctx.mem.lineWidth = scale;
ctx.rtt.lineWidth = scale;

ctx.cpu.strokeStyle = "#00F";
ctx.mem.strokeStyle = "#F00";
ctx.rtt.strokeStyle = "#0F0";

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
	ctx.cpu.clearRect(0, 0, width, height);
	ctx.mem.clearRect(0, 0, width, height);
	ctx.rtt.clearRect(0, 0, width, height);
}

function standardize(max0, max1, value) {
	return max0 - ((value / max1) * max0);
}

function updateCanvas(data) {
	backlog.push(data);
	clear();

	for(var i = 1; i < backlog.length; i++) {
		const prev = backlog[i - 1];
		const curr = backlog[i];

		const c0 = standardize(canvases.cpu.height, 100.0, prev.cpu);
		const c1 = standardize(canvases.cpu.height, 100.0, curr.cpu);
		const m0 = standardize(canvases.mem.height, 100.0, prev.mem);
		const m1 = standardize(canvases.mem.height, 100.0, curr.mem);
		const r0 = standardize(canvases.rtt.height, 100.0, prev.rtt);
		const r1 = standardize(canvases.rtt.height, 100.0, curr.rtt);

		ctx.cpu.beginPath();
		ctx.cpu.moveTo((i - 1) * scale, c0);
		ctx.cpu.lineTo(i * scale, c1);
		ctx.cpu.stroke();

		ctx.mem.beginPath();
		ctx.mem.moveTo((i - 1) * scale, m0);
		ctx.mem.lineTo(i * scale, m1);
		ctx.mem.stroke();

		ctx.rtt.beginPath();
		ctx.rtt.moveTo((i - 1) * scale, r0);
		ctx.rtt.lineTo(i * scale, r1);
		ctx.rtt.stroke();
	}
	
	if(backlog.length > maxBacklog) {
		backlog.shift();
	}
}
