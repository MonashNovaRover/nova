import {Card, CardBody, CardHeader, CardProps, Select, SelectItem} from "@nextui-org/react";
import React from "react";
import {Box, Droplet} from "react-feather";
import {useGenericStore} from "../../hooks/useGenericStore.ts";
import {SiteData, SiteDataState} from "../../redux/models/genericStores/SiteDataState.ts";
import {SpaceResourcesSiteType} from "../NIRProbe/SpaceResourcesSiteType.tsx";
import SiteSelectWidget from "./SiteSelectWidget.tsx";
import {Site} from "../../redux/models/genericStores/CurrentSiteStore.ts";

export interface SiteSelectWidgetProps extends CardProps {
  pickerClassName?: string,
}

const siteTypeSelectOptions = [
  {
    type: SpaceResourcesSiteType.WATER,
    name: "Water",
    icon: (<Droplet/>)
  },
  {
    type: SpaceResourcesSiteType.ILMENITE,
    name: "Ilmenite",
    icon: (<Box/>)
  }
]

const SiteTypeSelectWidget: React.FC<SiteSelectWidgetProps> = (
  {
    pickerClassName,
    ...cardProps
  }) => {

  const [currentSite, _] = useGenericStore<Site>("currentSite");
  const [siteData, setSiteData] = useGenericStore<SiteDataState>("siteData");

  const currentSiteType = siteData[currentSite].siteType;

  const onTypeChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    setSiteData(
      {
        ...siteData,
        [currentSite]: {
          ...siteData[currentSite],
          siteType: +e.target.value as SpaceResourcesSiteType,
        } as SiteData,
      }
    )
  }

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0 flex flex-row gap-3">
        <div className="grow">Site Select</div>
        <Select
          selectedKeys={[`${currentSiteType}`]}
          className="min-w-unit-32 w-48 shrink"
          size="md"
          labelPlacement="outside-left"
          onChange={onTypeChange}
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
        <SiteSelectWidget pickerClassName={pickerClassName} />
      </CardBody>
    </Card>
  );
};

export default SiteTypeSelectWidget;