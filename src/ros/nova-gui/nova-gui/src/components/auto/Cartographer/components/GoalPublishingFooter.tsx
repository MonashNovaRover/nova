import React from "react";
import { Button, Select, SelectItem, Input } from "@nextui-org/react";
import { Edit, Check } from "react-feather";
import { GoalType } from "../../../../redux/models/CartographerState.ts";

interface GoalPublishingFooterProps {
  goalType: GoalType;
  searchRadius: number;
  hasSelection: boolean;
  isEditing: boolean;
  onGoalTypeChange: (goalType: GoalType) => void;
  onSearchRadiusChange: (radius: number) => void;
  onToggleEditing: () => void;
  onPublish: () => void;
}

export const GoalPublishingFooter: React.FC<GoalPublishingFooterProps> = ({
  goalType,
  searchRadius,
  hasSelection,
  isEditing,
  onGoalTypeChange,
  onSearchRadiusChange,
  onToggleEditing,
  onPublish,
}) => {

  return (
    <div className="flex justify-between items-center gap-4 w-full">
      <div className="flex items-center gap-4">
        {hasSelection ? (
          <>
            <div className="flex items-center gap-6 text-sm text-gray-600 dark:text-gray-400">
              <span>
                Goal Type:{' '}
                {isEditing ? (
                  <Select
                    aria-label="goal-input"
                    selectedKeys={[String(goalType)]}
                    onChange={(e) => onGoalTypeChange(Number(e.target.value) as GoalType)}
                    className="inline-flex w-32"
                    size="sm"
                    variant="bordered"
                  >
                    <SelectItem key={String(GoalType.GNSS)}>GNSS</SelectItem>
                    <SelectItem key={String(GoalType.AR_TAG)}>AR Tag</SelectItem>
                    <SelectItem key={String(GoalType.OBJECT)}>Object</SelectItem>
                  </Select>
                ) : (
                  <span className="font-semibold">{GoalType[goalType]?.replace('_', ' ')}</span>
                )}
              </span>
              <span>
                Search Radius:{' '}
                {isEditing ? (
                  <Input
                    aria-label="search-input"
                    type="number"
                    value={String(searchRadius)}
                    min="0"
                    max="50"
                    step="1"
                    onChange={(e) => onSearchRadiusChange(Number(e.target.value))}
                    className="inline-flex w-20"
                    size="sm"
                    variant="bordered"
                  />
                ) : (
                  <span className="font-semibold">{searchRadius}m</span>
                )}
              </span>
            </div>
            <Button
              size="sm"
              variant="light"
              isIconOnly
              onPress={onToggleEditing}
            >
              {isEditing ? (
                <Check size={20} className="text-gray-600 dark:text-gray-400"/>
              ) : (
                <Edit size={20} className="text-gray-600 dark:text-gray-400"/>
              )}
            </Button>
          </>
        ) : (
          <span className="text-sm text-gray-600 dark:text-gray-400">
            Select points to publish
          </span>
        )}
      </div>
      <Button onPress={onPublish}>Publish</Button>
    </div>
  );
};
