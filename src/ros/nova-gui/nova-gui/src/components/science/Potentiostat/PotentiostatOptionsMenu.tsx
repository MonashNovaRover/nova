import {
  Button,
  Dropdown,
  DropdownItem,
  DropdownMenu,
  DropdownSection,
  DropdownTrigger,
} from "@nextui-org/react";
import { Check, MoreHorizontal, Trash2 } from "react-feather";

export interface PotentiostatOptionsMenuProps {
  lockButtonsDuringReading: boolean;
  onToggleLock: () => void;
  channel1Count: number;
  channel2Count: number;
  onClearChannel1: () => void;
  onClearChannel2: () => void;
  onClearAll: () => void;
}

export const PotentiostatOptionsMenu = ({
  lockButtonsDuringReading,
  onToggleLock,
  channel1Count,
  channel2Count,
  onClearChannel1,
  onClearChannel2,
  onClearAll,
}: PotentiostatOptionsMenuProps) => {
  return (
    <Dropdown>
      <DropdownTrigger>
        <Button variant="light" isIconOnly>
          <MoreHorizontal />
        </Button>
      </DropdownTrigger>
      <DropdownMenu aria-label="Potentiostat Options">
        <DropdownSection title="Settings" showDivider>
          <DropdownItem
            key="lockButtons"
            startContent={lockButtonsDuringReading ? <Check size={16} /> : <div className="w-4" />}
            onPress={onToggleLock}
          >
            Lock buttons during reading
          </DropdownItem>
        </DropdownSection>
        <DropdownSection title="Clear Data">
          <DropdownItem
            key="clearCh1"
            startContent={<Trash2 size={16} />}
            className="text-danger"
            color="danger"
            isDisabled={channel1Count === 0}
            onPress={onClearChannel1}
          >
            Clear Channel 1 ({channel1Count} pts)
          </DropdownItem>
          <DropdownItem
            key="clearCh2"
            startContent={<Trash2 size={16} />}
            className="text-danger"
            color="danger"
            isDisabled={channel2Count === 0}
            onPress={onClearChannel2}
          >
            Clear Channel 2 ({channel2Count} pts)
          </DropdownItem>
          <DropdownItem
            key="clearAll"
            startContent={<Trash2 size={16} />}
            className="text-danger"
            color="danger"
            isDisabled={channel1Count === 0 && channel2Count === 0}
            onPress={onClearAll}
          >
            Clear All Data
          </DropdownItem>
        </DropdownSection>
      </DropdownMenu>
    </Dropdown>
  );
};
