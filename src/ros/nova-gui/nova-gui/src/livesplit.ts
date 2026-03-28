const ws = new WebSocket("ws://localhost:16834/livesplit");

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
