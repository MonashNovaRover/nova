import {Card, CardBody, CardHeader, CardProps, Select, SelectItem} from "@nextui-org/react";
import React from "react";
import {Box, Droplet} from "react-feather";
import SpaceResourceSiteType from "../../../components/nir-probe/SpaceResourcesSiteType.tsx";
import SpaceResourcesSiteType from "../../../components/nir-probe/SpaceResourcesSiteType.tsx";
import SegmentedPicker from "../../../components/SegmentedPicker/SegmentedPicker.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";

export const siteFilenames = [
  "site1",
  "site2",
  "site3",
  "site4"
]

export interface SiteSelectWidgetProps extends CardProps {
  hideSiteType?: boolean,
  hideCard?: boolean,
  pickerClassName?: string,
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

const SiteSelectWidget: React.FC<SiteSelectWidgetProps> = (
  {
    hideSiteType,
    hideCard,
    pickerClassName,
    ...cardProps
  }) => {

  const [currentSite, setCurrentSite] = useGenericStore<number>("currentSite");
  const [currentSiteType, setCurrentSiteType] = useGenericStore<SpaceResourceSiteType>("currentSiteType");

  const picker = (
    <SegmentedPicker
      fullWidth
      className={"grow " + (pickerClassName ?? "")}
      onIndexChange={setCurrentSite}
      selectedIndex={currentSite}
    >
      <div className="text-rose-300">Site 1</div>
      <div className="text-amber-200">Site 2</div>
      <div className="text-sky-300">Site 3</div>
      <div className="text-violet-300">Site 4</div>
    </SegmentedPicker>
  );


  const card = (
    <Card {...cardProps}>
      <CardHeader className="pb-0 flex flex-row gap-3">
        <div className="grow">Site Select</div>
        {
          (hideSiteType ?? false) ? (<div></div>) : (
            <Select
              selectedKeys={[`${currentSiteType}`]}
              className="min-w-unit-32 w-48 shrink"
              size="md"
              labelPlacement="outside-left"
              onChange={e => setCurrentSiteType(+e.target.value as SpaceResourcesSiteType)}
              aria-label="Site Type"
              startContent={siteTypeSelectOptions[currentSiteType].icon}
            >
              {siteTypeSelectOptions.map(({type, name, icon}) => (
                <SelectItem key={`${type}`} value={type} startContent={icon}>
                  {name}
                </SelectItem>
              ))}
            </Select>
          )
        }
      </CardHeader>
      <CardBody className="flex flex-row gap-3">
        {picker}
      </CardBody>
    </Card>
  );

  return (hideCard ?? false) ? picker : card;
};

export default SiteSelectWidget;