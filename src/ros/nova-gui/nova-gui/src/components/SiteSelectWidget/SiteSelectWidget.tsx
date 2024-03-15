import {Card, CardBody, CardHeader, CardProps, Select, SelectItem} from "@nextui-org/react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";
import React from "react";
import {Box, Droplet} from "react-feather";
import SpaceResourceSiteType from "../nir-probe/SpaceResourcesSiteType.tsx";
import SpaceResourcesSiteType from "../nir-probe/SpaceResourcesSiteType.tsx";

export const siteFilenames = [
  "site1",
  "site2",
  "site3",
  "site4"
]

export interface SiteSelectWidgetProps extends CardProps {
  onValueChanged?: (value: string) => void,
  onSiteTypeChanged?: (newType: SpaceResourceSiteType) => void,
  currentSiteType: SpaceResourceSiteType,
}

const siteTypeSelectOptions = [
  {
    type: SpaceResourceSiteType.WATER,
    name: "Water",
    icon: (<Droplet/>)
  },
  {
    type: SpaceResourceSiteType.ILMENITE,
    name: "Ilmenite",
    icon: (<Box/>)
  }
]

const SiteSelectWidget: React.FC<SiteSelectWidgetProps> = ({
  onValueChanged, currentSiteType, onSiteTypeChanged, ...cardProps
}) => {

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0 flex flex-row gap-3">
        <div className="grow">Site Select</div>
        <Select
          selectedKeys={[`${currentSiteType}`]}
          className="min-w-unit-32 w-48 shrink"
          size="md"
          labelPlacement="outside-left"
          onChange={e => onSiteTypeChanged?.(+e.target.value as SpaceResourcesSiteType)}
          aria-label="Site Type"
          startContent={siteTypeSelectOptions[currentSiteType].icon}
        >
          {siteTypeSelectOptions.map(({type, name, icon}) => (
            <SelectItem key={`${type}`} value={type} startContent={icon}>
              {name}
            </SelectItem>
          ))}
        </Select>
      </CardHeader>
      <CardBody className="flex flex-row gap-3">

        <SegmentedPicker fullWidth
                         className="grow"
                         onIndexChange={(i) => onValueChanged?.(siteFilenames[i])}>
          <div className="text-rose-300">Site 1</div>
          <div className="text-amber-200">Site 2</div>
          <div className="text-sky-300">Site 3</div>
          <div className="text-violet-300">Site 4</div>
        </SegmentedPicker>

      </CardBody>
    </Card>
  );
};

export default SiteSelectWidget;