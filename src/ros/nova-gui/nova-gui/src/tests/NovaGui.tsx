import React from "react";
import ReactDOM from "react-dom/client";
import "../index.css";
import { RouterProvider, createBrowserRouter } from "react-router-dom";
import { routes } from "../routes/routes";

export default function App() {
  const router = createBrowserRouter(routes);
  return <RouterProvider router={router} />;
}
