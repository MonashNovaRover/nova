import {Card, CardBody, CardHeader, CardProps, Select, SelectItem} from "@nextui-org/react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";
import React from "react";
import {Circle, Droplet} from "react-feather";
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
    icon: (<Circle/>)
  }
]

const SiteSelectWidget: React.FC<SiteSelectWidgetProps> = ({
  onValueChanged, currentSiteType, onSiteTypeChanged, ...cardProps
}) => {

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        Site Select
      </CardHeader>
      <CardBody className="flex flex-row gap-3">

        <SegmentedPicker fullWidth
                         className="grow"
                         onIndexChange={(i) => onValueChanged?.(siteFilenames[i])}>
          <>Site 1</>
          <>Site 2</>
          <>Site 3</>
          <>Site 4</>
        </SegmentedPicker>

        <Select
          selectedKeys={[`${currentSiteType}`]}
          className="w-32"
          size="md"
          labelPlacement="outside-left"
          onChange={e => onSiteTypeChanged?.(+e.target.value as SpaceResourcesSiteType)}
          aria-label="Site Type"
        >
          {siteTypeSelectOptions.map(({type, name, icon}) => (
            <SelectItem key={`${type}`} value={type} startContent={icon}>
              {name}
            </SelectItem>
          ))}
        </Select>

      </CardBody>
    </Card>
  );
};

export default SiteSelectWidget;