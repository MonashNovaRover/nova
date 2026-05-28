import { useGenericStore } from "../../../../hooks/useGenericStore.ts";
import { MapCoordinate} from "../../../../redux/models/CartographerState.ts";

export type DisplayMapCoordinate = {
    lat: string,
    long: string
}

type DMS = {
    degrees: number
    minutes: number
    seconds: number
    direction: string
}

type DDM = {
    degrees: number
    minutes: number
    direction: string
}

function DDtoDMS(dd: number, isLat: boolean | undefined = undefined): DMS {
    const absDD = Math.abs(dd);
    const degrees = Math.floor(absDD);
    const minutes = Math.floor((absDD - degrees) * 60);
    const seconds = ((absDD - degrees - minutes / 60) * 3600).toFixed(2);

    return {
        degrees: degrees,
        minutes: minutes,
        seconds: parseFloat(seconds),
        direction: isLat === undefined 
            ? (dd >= 0 ? "N/E" : "S/W") 
            : (isLat 
                ? (dd >= 0 ? "N" : "S") 
                : (dd >= 0 ? "E" : "W"))
    };
}

function DDtoDDM(dd: number, isLat: boolean | undefined = undefined): DDM {
    const absDD = Math.abs(dd);
    const degrees = Math.floor(absDD);
    const minutes = (absDD - degrees) * 60;

    return {
        degrees: degrees,
        minutes: minutes,
        direction: isLat === undefined 
            ? (dd >= 0 ? "N/E" : "S/W") 
            : (isLat 
                ? (dd >= 0 ? "N" : "S") 
                : (dd >= 0 ? "E" : "W"))
    };
}

function DMStoDD(dms: DMS): number {
    const dd = dms.degrees + (dms.minutes / 60) + (dms.seconds / 3600);
    const finalDD = dms.direction.toUpperCase() === 'S' || dms.direction.toUpperCase() === 'W' ? dd * -1 : dd;
    return finalDD
}

function DDMtoDD(ddm: DDM): number {
    const dd = ddm.degrees + (ddm.minutes / 60);
    const finalDD = ddm.direction.toUpperCase() === 'S' || ddm.direction.toUpperCase() === 'W' ? dd * -1 : dd;
    return finalDD
}

function calcMapCoordinateFromDD({lat, long}: DisplayMapCoordinate): MapCoordinate {
    return {lat: +lat.slice(0,-1), long: +long.slice(0,-1)} as MapCoordinate
}

function calcMapCoordinateFromDMS({lat, long}: DisplayMapCoordinate): MapCoordinate {
    const latArr = lat.split(" ").map((v, i, a)=>a.length-1 !== i ? +v.slice(0,1) : v)
    const latDMS = {degrees: latArr[0], minutes: latArr[1], seconds: latArr[2], direction: latArr[3]} as DMS
    const longArr = long.split(" ").map((v, i, a)=>a.length-1 !== i ? +v.slice(0,1) : v)
    const longDMS = {degrees: longArr[0], minutes: longArr[1], seconds: longArr[2], direction: longArr[3]} as DMS
    const calcLat = DMStoDD(latDMS)
    const calcLong = DMStoDD(longDMS);
    return {
        lat: calcLat,
        long: calcLong
    }
}

function calcMapCoordinateFromDDM({lat, long}: DisplayMapCoordinate): MapCoordinate {
    const latArr = lat.split(" ").map((v, i, a)=>a.length-1 !== i ? +v.slice(0,1) : v)
    const latDMS = {degrees: latArr[0], minutes: latArr[1], direction: latArr[2]} as DDM
    const longArr = long.split(" ").map((v, i, a)=>a.length-1 !== i ? +v.slice(0,1) : v)
    const longDMS = {degrees: longArr[0], minutes: longArr[1], direction: longArr[2]} as DDM
    const calcLat = DDMtoDD(latDMS)
    const calcLong = DDMtoDD(longDMS);
    return {
        lat: calcLat,
        long: calcLong
    }
}

function useCalcMapCoordinate(coord: DisplayMapCoordinate): MapCoordinate {
    const [cartographerCoordinateFormat] = useGenericStore<number>("cartographerCoordinateFormat");
    switch (cartographerCoordinateFormat) {
        /** numbers align with coordinateFormatOptions, there should be a better way to do this from "../../../navbar/settings/CartographerSettings.tsx"*/
        case 0:
            return calcMapCoordinateFromDD(coord);
        case 1:
            return calcMapCoordinateFromDMS(coord);
        case 2:
            return calcMapCoordinateFromDDM(coord);
        default:
            return calcMapCoordinateFromDD(coord);
    };
}


function displayMapCoordinateAsDD({lat, long}: MapCoordinate): DisplayMapCoordinate {
    return {lat: lat.toString()+"°", long: long.toString()+"°"} as DisplayMapCoordinate
}

function displayMapCoordinateAsDMS({lat, long}: MapCoordinate): DisplayMapCoordinate {
    const dmsLat = DDtoDMS(lat, true);
    const dmsLong = DDtoDMS(long, false);
    return {
        lat: `${dmsLat.degrees}° ${dmsLat.minutes}' ${dmsLat.seconds}" ${dmsLat.direction}`,
        long: `${dmsLong.degrees}° ${dmsLong.minutes}' ${dmsLong.seconds}" ${dmsLong.direction}`
    }
}

function displayMapCoordinateAsDDM({lat, long}: MapCoordinate): DisplayMapCoordinate {
    const dmsLat = DDtoDDM(lat, true);
    const dmsLong = DDtoDDM(long, false);
    return {
        lat: `${dmsLat.degrees}° ${dmsLat.minutes}' ${dmsLat.direction}`,
        long: `${dmsLong.degrees}° ${dmsLong.minutes}' ${dmsLong.direction}`
    }
}

function useDisplayMapCoordinate(coord: MapCoordinate): DisplayMapCoordinate {
    const [cartographerCoordinateFormat] = useGenericStore<number>("cartographerCoordinateFormat");
    switch (cartographerCoordinateFormat) {
        /** numbers align with coordinateFormatOptions, there should be a better way to do this from "../../../navbar/settings/CartographerSettings.tsx"*/
        case 0:
            return displayMapCoordinateAsDD(coord);
        case 1:
            return displayMapCoordinateAsDMS(coord);
        case 2:
            return displayMapCoordinateAsDDM(coord);
        default:
            return displayMapCoordinateAsDD(coord);
    };
}

export {useDisplayMapCoordinate, useCalcMapCoordinate}