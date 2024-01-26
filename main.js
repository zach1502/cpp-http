// make an animation of a small red ball bouncing around the screen
// the ball should start at the top left corner of the screen
// the ball should bounce off the edges of the screen
// the ball should change color every time it bounces
// the ball should change direction randomly every time it bounces


// create a canvas to draw on
var canvas = document.createElement("canvas");
canvas.width = 400;
canvas.height = 400;
document.body.appendChild(canvas);

// get the context
var ctx = canvas.getContext("2d");

// create a ball object
var ball = {
  x: 25,
  y: 25,
  radius: 20,
  color: "red",
  dx: 4,
  dy: 4
};

// draw the ball
function drawBall() {
  ctx.beginPath();
  ctx.arc(ball.x, ball.y, ball.radius, 0, Math.PI * 2);
  ctx.fillStyle = ball.color;
  ctx.fill();
  ctx.closePath();
}

// update the ball's position
function updateBall() {
  ball.x += ball.dx;
  ball.y += ball.dy;
  if (ball.x + ball.radius > canvas.width || ball.x - ball.radius < 0) {
    ball.dx = -ball.dx + Math.random() * ball.dx - ball.dx/2;
    ball.color = randomColor();
  }
  if (ball.y + ball.radius > canvas.height || ball.y - ball.radius < 0) {
    ball.dy = -ball.dy + Math.random() * ball.dy - ball.dy/2;
    ball.color = randomColor();
  }

  // normalize the direction vector
  var length = Math.sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
  ball.dx /= length /2;
  ball.dy /= length /2;
}

// clear the canvas
function clearCanvas() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
}

// draw the ball and update its position

function draw() {
    clearCanvas();
    drawBall();
    updateBall();
    }

// run the animation
setInterval(draw, 10);

// generate a random color
function randomColor() {
  var colors = ["red", "orange", "yellow", "green", "blue", "purple"];
  var randomIndex = Math.floor(Math.random() * colors.length);
  return colors[randomIndex];
}

// collision detection
function isColliding() {
  var distance = Math.sqrt(Math.pow(ball.x - mouse.x, 2) + Math.pow(ball.y - mouse.y, 2));
  return distance < ball.radius;
}

// add a mousemove event listener to the canvas

canvas.addEventListener("mousemove", function(event) {
    mouse.x = event.clientX;
    mouse.y = event.clientY;
    if (isColliding()) {
        ball.color = randomColor();
    }
    }
);
