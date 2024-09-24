import { useNavigate } from "react-router-dom";
import { NextUIProvider } from "@nextui-org/react";
import configureRootStore from "./redux/store/configureRootStore";
import { Provider } from "react-redux";
import { RosRoot } from "./RosRoot";
import { PersistGate } from "redux-persist/integration/react";

export const Root = () => {
  const navigate = useNavigate();
  const {store, persistor} = configureRootStore();

  return (
    <NextUIProvider navigate={navigate}>
      <Provider store={store}>
        <PersistGate loading={null} persistor={persistor}>
          <RosRoot/>
        </PersistGate>
      </Provider>
    </NextUIProvider>
  );
};
