import { Select, SelectItem, SharedSelection } from "@nextui-org/react";
import { useGenericStore } from "../../../hooks/useGenericStore";

export type coordinateFormatOptionsType = {
  id: number,  
  label: string,
  name: string,
};

export const coordinateFormatOptions = [
  {id: 0, label: "DD", name: "Decimal Degrees"},
  {id: 1, label: "DMS", name: "Degrees, Minutes and Seconds"},
  {id: 2, label: "DDM", name: "Degrees, Decimal Minutes"},
] as coordinateFormatOptionsType[];

export default function CartographerSettings() {
  const [coordinateFormat, setCoordinateFormat] = useGenericStore<number>("cartographerCoordinateFormat");

  const onFormatSelectionChange = (keys: SharedSelection) => {
    const selected = Array.from(keys)[0];
    setCoordinateFormat(+selected);
  };
  return (
    <div className="flex flex-col gap-3 mt-2 mb-4">
      <Select
        size="sm"
        label="Latitude & Longitude Format"
        selectedKeys={[coordinateFormat.toString()]}
        onSelectionChange={onFormatSelectionChange}
      >
        {coordinateFormatOptions.map((option) => (
          <SelectItem key={option.id}>
            {`${option.label}: ${option.name}`}
          </SelectItem>
        ))}
      </Select>

      <div className="text-xs text-default-500">
        Current coordinate format: {coordinateFormatOptions[coordinateFormat].label}
      </div>

      <div className="text-xs text-default-500">
        Logs include batch, preprocessms, runms, postms, and totalms in the browser console.
      </div>
    </div>
  );
}