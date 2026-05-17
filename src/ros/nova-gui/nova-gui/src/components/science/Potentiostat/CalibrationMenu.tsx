import { useState } from "react";
import {
  Button,
  Dropdown,
  DropdownItem,
  DropdownMenu,
  DropdownSection,
  DropdownTrigger,
} from "@nextui-org/react";
import { Check, Settings, Trash2, Sliders } from "react-feather";
import type { CalibrationState } from "./potentiostatStorage.ts";
import { ManualOffsetDialog } from "./ManualOffsetDialog.tsx";

type WidgetMode = "measurement" | "calibration";

export interface CalibrationMenuProps {
  mode: WidgetMode;
  onSetMode: (mode: WidgetMode) => void;
  calibration: CalibrationState;
  onSetManualOffset: (channel: 1 | 2, voltage: number, current: number) => void;
  onClearCalibration: (channel: 1 | 2) => void;
}

function getCalibrationDescription(offsets: CalibrationState["channel1"]): string {
  if (!offsets) return "Not calibrated";
  const source = offsets.isManual ? "Manual" : "Calculated";
  const date = new Date(offsets.timestamp).toLocaleDateString();
  return `${source} - ${date}`;
}

export const CalibrationMenu = ({
  mode,
  onSetMode,
  calibration,
  onSetManualOffset,
  onClearCalibration,
}: CalibrationMenuProps) => {
  const [manualOffsetChannel, setManualOffsetChannel] = useState<1 | 2 | null>(null);

  return (
    <>
      <Dropdown>
        <DropdownTrigger>
          <Button variant="light" isIconOnly>
            <Sliders />
          </Button>
        </DropdownTrigger>
        <DropdownMenu aria-label="Calibration Options">
          <DropdownSection title="Mode" showDivider>
            <DropdownItem
              key="measurementMode"
              textValue="Measurement Mode"
              startContent={mode === "measurement" ? <Check size={16} /> : <div className="w-4" />}
              onPress={() => onSetMode("measurement")}
            >
              Measurement Mode
            </DropdownItem>
            <DropdownItem
              key="calibrationMode"
              textValue="Calibration Mode"
              startContent={mode === "calibration" ? <Check size={16} /> : <div className="w-4" />}
              onPress={() => onSetMode("calibration")}
            >
              Calibration Mode
            </DropdownItem>
          </DropdownSection>

          <DropdownSection title="Channel 1 Calibration" showDivider>
            <DropdownItem
              key="ch1Status"
              textValue="Channel 1 Status"
              description={getCalibrationDescription(calibration.channel1)}
              isReadOnly
            >
              {calibration.channel1
                ? `V: ${calibration.channel1.voltageOffset.toFixed(4)}V, C: ${calibration.channel1.currentOffset.toFixed(4)}mA`
                : "Not calibrated"}
            </DropdownItem>
            <DropdownItem
              key="ch1Manual"
              textValue="Set Manual Offset"
              startContent={<Settings size={16} />}
              onPress={() => setManualOffsetChannel(1)}
            >
              Set Manual Offset
            </DropdownItem>
            <DropdownItem
              key="ch1Clear"
              textValue="Clear Calibration"
              startContent={<Trash2 size={16} />}
              className="text-danger"
              color="danger"
              isDisabled={!calibration.channel1}
              onPress={() => onClearCalibration(1)}
            >
              Clear Calibration
            </DropdownItem>
          </DropdownSection>

          <DropdownSection title="Channel 2 Calibration">
            <DropdownItem
              key="ch2Status"
              textValue="Channel 2 Status"
              description={getCalibrationDescription(calibration.channel2)}
              isReadOnly
            >
              {calibration.channel2
                ? `V: ${calibration.channel2.voltageOffset.toFixed(4)}V, C: ${calibration.channel2.currentOffset.toFixed(4)}mA`
                : "Not calibrated"}
            </DropdownItem>
            <DropdownItem
              key="ch2Manual"
              textValue="Set Manual Offset"
              startContent={<Settings size={16} />}
              onPress={() => setManualOffsetChannel(2)}
            >
              Set Manual Offset
            </DropdownItem>
            <DropdownItem
              key="ch2Clear"
              textValue="Clear Calibration"
              startContent={<Trash2 size={16} />}
              className="text-danger"
              color="danger"
              isDisabled={!calibration.channel2}
              onPress={() => onClearCalibration(2)}
            >
              Clear Calibration
            </DropdownItem>
          </DropdownSection>
        </DropdownMenu>
      </Dropdown>

      {manualOffsetChannel && (
        <ManualOffsetDialog
          isOpen={manualOffsetChannel !== null}
          onClose={() => setManualOffsetChannel(null)}
          channel={manualOffsetChannel}
          currentOffsets={
            calibration[manualOffsetChannel === 1 ? "channel1" : "channel2"]
          }
          onSave={(voltage, current) => {
            onSetManualOffset(manualOffsetChannel, voltage, current);
            setManualOffsetChannel(null);
          }}
        />
      )}
    </>
  );
};
