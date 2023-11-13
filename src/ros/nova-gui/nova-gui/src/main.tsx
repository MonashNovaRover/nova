import React from "react";
import ReactDOM from "react-dom/client";
import "./index.css";
import { Root } from "./root";
import {
  BrowserRouter,
  RouterProvider,
  createBrowserRouter,
} from "react-router-dom";
import { routes } from "./routes/routes";

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);

function App() {
  const router = createBrowserRouter(routes);

  return <RouterProvider router={router} />;
}
