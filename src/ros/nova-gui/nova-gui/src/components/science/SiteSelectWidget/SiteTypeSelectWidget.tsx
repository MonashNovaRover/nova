import {Card, CardContent, CardHeader, CardProps, ListBox, ListBoxItem, Select} from "@heroui/react";
import React from "react";
import {Box, Droplet} from "react-feather";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {SiteData, SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";
import {SpaceResourcesSiteType} from "../NIRProbe/SpaceResourcesSiteType.tsx";
import SiteSelectWidget from "./SiteSelectWidget.tsx";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";

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
          aria-label="Site Type"
          onSelectionChange={(key) => onTypeChange({ target: { value: String(key) } } as React.ChangeEvent<HTMLSelectElement>)}
        >
          <Select.Trigger>
            <Select.Value />
            <Select.Indicator />
          </Select.Trigger>
          <Select.Popover>
            <ListBox>
              {siteTypeSelectOptions.map(({type, name, icon}) => (
                <ListBoxItem id={`${type}`} key={`${type}`} textValue={name}>
                  <span className="flex items-center gap-2">{icon}{name}</span>
                </ListBoxItem>
              ))}
            </ListBox>
          </Select.Popover>
        </Select>
      </CardHeader>
      <CardContent className="flex flex-row gap-3">
        <SiteSelectWidget pickerClassName={pickerClassName} />
      </CardContent>
    </Card>
  );
};

export default SiteTypeSelectWidget;