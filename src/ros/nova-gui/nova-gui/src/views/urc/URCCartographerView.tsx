import { APIProvider } from "@vis.gl/react-google-maps";
import { MapCanvas } from "../../components/Maps/MapCanvas";

export const URCCartographerView = () => {
  return (
    <>
      <APIProvider apiKey={"AIzaSyAHBHVjWPwibfLTFqf6PZQVdj_5mbQGwyA"}>
        <MapCanvas />
      </APIProvider>
      {/* <MapOverlay /> */}
    </>
  );
};
