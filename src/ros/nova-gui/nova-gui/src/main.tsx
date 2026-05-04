// --- perf instrumentation (remove after benchmarking) ---
const __t0 = performance.now();
const __origParse = JSON.parse;
let __parseCount = 0;
let __parseBytes = 0;
JSON.parse = function (text: string, reviver?: (this: unknown, key: string, value: unknown) => unknown) {
  __parseCount++;
  __parseBytes += typeof text === "string" ? text.length : 0;
  return __origParse(text, reviver);
};
interface PerfApi {
  bootMs: () => number;
  parses: () => number;
  bytesParsed: () => number;
  reset: () => void;
}
(window as unknown as { perf: PerfApi }).perf = {
  bootMs: () => performance.now() - __t0,
  parses: () => __parseCount,
  bytesParsed: () => __parseBytes,
  reset: () => { __parseCount = 0; __parseBytes = 0; },
};
// --- end perf instrumentation ---

import React from "react";
import ReactDOM from "react-dom/client";
import "./index.css";
import { RouterProvider, createBrowserRouter } from "react-router-dom";
import { routes } from "./routes/routes";
import configureRootStore from "./redux/store/configureRootStore";
import { Provider } from "react-redux";

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);

function App() {
  const router = createBrowserRouter(routes);
  const {store} = configureRootStore();

  React.useEffect(() => {
    const perf = (window as unknown as { perf: PerfApi }).perf;
    console.log("[perf] first paint:", perf.bootMs().toFixed(0), "ms");
    console.log("[perf] parses at first paint:", perf.parses());
    console.log("[perf] bytes parsed:", perf.bytesParsed());
  }, []);

  return (
    <Provider store={store}>
      <RouterProvider router={router} />
    </Provider>
  );

}
