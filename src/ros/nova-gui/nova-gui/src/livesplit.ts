const ws = new WebSocket("ws://localhost:16834/livesplit");

const intervalId = setInterval(() => {
  console.log("checking time");
  const now = new Date();
  if (now.getHours() === 9 && now.getMinutes() === 55) {
    for (let i = 0; i < 16; i++) {
      ws.send("startorsplit");
  };
  }
}, 10000);

export default (taskname: string) => {
  if (taskname == "clear") {
    localStorage.clear();
    return;
  }

  if (!localStorage.getItem("task_"+taskname)) {
    ws.send("startorsplit");
    localStorage.setItem("task_"+taskname, "true");
  }
}
