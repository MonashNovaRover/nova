import { Cartographer } from "../../components/maps/Cartographer/Cartographer.tsx";

export const URCCartographerView = () => {
  return (
    <div className="w-full overflow-hidden" style={{ height: "calc(100vh - 4.01rem)" }}>
      <Cartographer />
    </div>
  );
};
