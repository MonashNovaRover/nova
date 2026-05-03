import React from "react";
import { useSortable } from "@dnd-kit/sortable";
import { CSS } from "@dnd-kit/utilities";
import { Card, Checkbox, Chip } from "@nextui-org/react";
import { GoalType, MapPoint } from "../../../../redux/models/CartographerState.ts";

interface SortableItemProps {
  id: string;
  isSpecial: boolean
  children: React.ReactNode;
}

export const SortablePoints: React.FC<SortableItemProps> = ({ id, isSpecial, children }) => {
  const { attributes, listeners, setNodeRef, transform, transition } =
    useSortable({ id });

  const style = {
    transform: CSS.Transform.toString(transform),
    transition,
    ...(isSpecial && { border: '2px solid #ec4899' }), // pink-500
  };

  return (
    <Card
      ref={setNodeRef}
      style={style}
      className="p-4 mb-4 rounded-lg bg-gray-100 bg-neutral-800"
      {...attributes}
      {...listeners}
    >
      {children}
    </Card>
  );
};

// GoalPointCard Component
interface GoalPointCardProps {
  point: MapPoint;
  isSelected: boolean;
  onToggleSelection: (name: string) => void;
}

export const GoalPointCard: React.FC<GoalPointCardProps> = ({
  point,
  isSelected,
  onToggleSelection,
}) => {
  return (
    <div className="grid grid-cols-7 items-center">
      <Checkbox
        isSelected={isSelected}
        onChange={() => onToggleSelection(point.name)}
      >
        <span className="flex-shrink-0 font-bold">{point.name}</span>
      </Checkbox>
      <span className="flex-shrink-0 col-span-2">{point.lat}</span>
      <span className="flex-shrink-0 col-span-2">{point.long}</span>
      <span className="flex-shrink-0">
        {point.searchRadius && `${point.searchRadius ?? 0}m`}
      </span>
      <div className="flex-shrink-0 flex justify-end w-24">
        {point.labelNumber !== null && GoalType[point.labelNumber] && (
          <Chip
            className={`ml-2 ${
              point.labelNumber == GoalType.GNSS
                ? "bg-blue-200 text-blue-800 dark:bg-blue-700 dark:text-blue-200"
                : point.labelNumber == GoalType.AR_TAG
                ? "bg-green-200 text-green-800 dark:bg-green-700 dark:text-green-200"
                : point.labelNumber == GoalType.OBJECT
                ? "bg-yellow-200 text-yellow-800 dark:bg-yellow-700 dark:text-yellow-200"
                : "bg-gray-200 text-gray-800 dark:bg-gray-700 dark:text-gray-200"
            }`}
            size="sm"
            radius="sm"
          >
            {point.labelName}
          </Chip>
        )}
      </div>
    </div>
  );
};