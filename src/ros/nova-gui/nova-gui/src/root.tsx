import { useNavigate } from "react-router-dom";
import { NextUIProvider } from "@nextui-org/react";
import configureRootStore from "./redux/store/configureRootStore";
import { Provider } from "react-redux";
import { RosRoot } from "./RosRoot";

export const Root = () => {
  const navigate = useNavigate();
  const store = configureRootStore();

  return (
    <NextUIProvider navigate={navigate}>
      <Provider store={store}>
        <RosRoot />
      </Provider>
    </NextUIProvider>
  );
};
