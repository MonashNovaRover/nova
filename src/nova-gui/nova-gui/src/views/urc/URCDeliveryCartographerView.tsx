import { Cartographer } from "../../components/maps/Cartographer/Cartographer.tsx";

export const URCDeliveryCartographerView = () => {
  return (
    <div className="w-full overflow-hidden" style={{ height: "calc(100vh - 4.01rem)" }}>
      <Cartographer enableDroneTracking={true} />
    </div>
  );
};
