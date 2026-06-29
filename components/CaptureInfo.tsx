import React, {useState, useEffect} from "react"
import * as JUCE from "juce-framework-frontend-mirror"
import XButton from "../assets/x-button.svg"
import "./styles/captureinfo.scss"

const getCaptureBars = JUCE.getNativeFunction("getCaptureBars")
const deleteCapture = JUCE.getNativeFunction("deleteCapture")

const CaptureInfo: React.FunctionComponent = () => {
    const [bars, setBars] = useState(0)

    useEffect(() => {
        initCaptureBars()
        const eventID = window.__JUCE__.backend.addEventListener("captureBars", updateCaptureBars)

        return () => {
            window.__JUCE__.backend.removeEventListener(eventID)
        }
    }, [])

    const initCaptureBars = async () => {
        const bars = await getCaptureBars()
        setBars(bars)
    }

    const updateCaptureBars = (bars: number) => {
        setBars(bars)
    }

    const handleDelete = async () => {
        await deleteCapture()
    }

    if (bars === 0) return null

    return (
        <div className="capture-info">
            <span className="capture-info-text">
                {bars} {bars === 1 ? "bar" : "bars"} stored
            </span>
            <XButton className="capture-info-icon" onClick={handleDelete}/>
        </div>
    )
}

export default CaptureInfo