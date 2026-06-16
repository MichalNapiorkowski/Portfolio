from flask import Flask, Response
from picamera2 import Picamera2
import cv2
import numpy as np
import time


app = Flask(__name__)

ROI_PATH = "/home/raspberry/Pictures/photo_conveyor_roi.jpg"
ROI_SWAP_BR_PATH = "/home/raspberry/Pictures/photo_conveyor_roi_swap_br.jpg"

PIXELS_FOR_27_MM = 720.0
MM_FOR_720_PIXELS = 27.0
MM_PER_PIXEL = MM_FOR_720_PIXELS / PIXELS_FOR_27_MM

MIN_OBJECT_AREA_PX = 8000
MIN_OBJECT_SHORT_SIDE_PX = 70
MAX_OBJECT_SIDE_PX = 500
MIN_RECTANGULARITY = 0.55
MIN_BLACK_BORDER_RATIO = 0.40
MIN_COLOR_SATURATION = 50
MIN_OBJECT_VALUE = 70
BLACK_VALUE_THRESHOLD = 95
BLACK_BORDER_MARGIN_PX = 18
OBJECT_MASK_ERODE_ITERATIONS = 2
MIN_VALUE_DELTA_TO_BACKGROUND = 50
MIN_SATURATION_DELTA_TO_BACKGROUND = 25


def fixed_conveyor_roi(image):
    x1 = 100
    x2 = 1000
    y1 = 0
    y2 = 720

    return image[y1:y2, x1:x2]


def save_roi_image(image):
    cv2.imwrite(ROI_PATH, image, [cv2.IMWRITE_JPEG_QUALITY, 90])

    swapped = image.copy()
    swapped[:, :, [0, 2]] = swapped[:, :, [2, 0]]
    cv2.imwrite(ROI_SWAP_BR_PATH, swapped, [cv2.IMWRITE_JPEG_QUALITY, 90])


def capture_and_save_roi():
    frame_rgb = picam2.capture_array()
    frame_bgr = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
    roi_bgr = fixed_conveyor_roi(frame_bgr)
    save_roi_image(roi_bgr)
    return roi_bgr


def add_white_border(image, border):
    return cv2.copyMakeBorder(
        image,
        border,
        border,
        border,
        border,
        cv2.BORDER_CONSTANT,
        value=(255, 255, 255),
    )


detector = cv2.QRCodeDetector()
detector.setEpsX(0.5)
detector.setEpsY(0.5)


def decode_qr(roi_bgr):
    gray = cv2.cvtColor(roi_bgr, cv2.COLOR_BGR2GRAY)
    clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8)).apply(gray)
    high_contrast = cv2.convertScaleAbs(clahe, alpha=1.6, beta=-35)

    border_size = 120
    roi_border = add_white_border(roi_bgr, border_size)
    gray_border = cv2.cvtColor(roi_border, cv2.COLOR_BGR2GRAY)
    clahe_border = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8)).apply(gray_border)
    high_contrast_border = cv2.convertScaleAbs(clahe_border, alpha=1.6, beta=-35)

    _, otsu_border = cv2.threshold(
        high_contrast_border,
        0,
        255,
        cv2.THRESH_BINARY + cv2.THRESH_OTSU,
    )

    adaptive_border = cv2.adaptiveThreshold(
        high_contrast_border,
        255,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY,
        41,
        7,
    )

    variants = [
        (gray, 0, 0, 1.0),
        (otsu_border, -border_size, -border_size, 1.0),
        (adaptive_border, -border_size, -border_size, 1.0),
    ]

    for image, offset_x, offset_y, scale in variants:
        data, points, _ = detector.detectAndDecode(image)

        if data:
            if points is not None:
                points = points.copy()
                points[:, :, 0] = (points[:, :, 0] / scale) + offset_x
                points[:, :, 1] = (points[:, :, 1] / scale) + offset_y

            return data, points

    return "", None


def annotate_qr(roi_bgr, qr_data, points):
    if points is None:
        return

    annotated = roi_bgr.copy()
    height, width = annotated.shape[:2]

    points = points.astype(np.int32)
    points[:, :, 0] = np.clip(points[:, :, 0], 0, width - 1)
    points[:, :, 1] = np.clip(points[:, :, 1], 0, height - 1)

    cv2.polylines(annotated, [points], True, (0, 255, 0), 3)

    x = int(np.min(points[:, :, 0]))
    y = int(np.min(points[:, :, 1]))

    cv2.putText(
        annotated,
        qr_data,
        (max(10, x), max(30, y - 10)),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (0, 255, 0),
        2,
        cv2.LINE_AA,
    )

    save_roi_image(annotated)


def has_black_border(gray, x, y, width, height):
    margin = BLACK_BORDER_MARGIN_PX
    img_h, img_w = gray.shape[:2]

    outer_x1 = max(0, x - margin)
    outer_y1 = max(0, y - margin)
    outer_x2 = min(img_w, x + width + margin)
    outer_y2 = min(img_h, y + height + margin)

    if outer_x1 == x or outer_y1 == y or outer_x2 == x + width or outer_y2 == y + height:
        return False

    outer = np.zeros(gray.shape, dtype=np.uint8)
    inner = np.zeros(gray.shape, dtype=np.uint8)
    outer[outer_y1:outer_y2, outer_x1:outer_x2] = 255
    inner[y:y + height, x:x + width] = 255
    border = cv2.subtract(outer, inner)

    border_pixels = gray[border == 255]
    if border_pixels.size == 0:
        return False

    black_ratio = np.mean(border_pixels < BLACK_VALUE_THRESHOLD)
    return black_ratio >= MIN_BLACK_BORDER_RATIO


def contour_touches_roi_border(x, y, width, height, roi_width, roi_height):
    margin = 4
    return (
        x <= margin
        or y <= margin
        or x + width >= roi_width - margin
        or y + height >= roi_height - margin
    )


def has_colored_inside(hsv, contour):
    mask = np.zeros(hsv.shape[:2], dtype=np.uint8)
    cv2.drawContours(mask, [contour], 0, 255, -1)

    s = hsv[:, :, 1][mask == 255]
    v = hsv[:, :, 2][mask == 255]

    if s.size == 0 or v.size == 0:
        return False

    return np.mean(s) >= MIN_COLOR_SATURATION and np.mean(v) >= MIN_OBJECT_VALUE


def has_object_background_contrast(hsv, gray, contour, x, y, width, height):
    margin = BLACK_BORDER_MARGIN_PX
    img_h, img_w = gray.shape[:2]

    outer_x1 = max(0, x - margin)
    outer_y1 = max(0, y - margin)
    outer_x2 = min(img_w, x + width + margin)
    outer_y2 = min(img_h, y + height + margin)

    contour_mask = np.zeros(gray.shape, dtype=np.uint8)
    cv2.drawContours(contour_mask, [contour], 0, 255, -1)
    inner_mask = cv2.erode(contour_mask, np.ones((5, 5), np.uint8), iterations=1)

    outer_mask = np.zeros(gray.shape, dtype=np.uint8)
    outer_mask[outer_y1:outer_y2, outer_x1:outer_x2] = 255
    ring_mask = cv2.subtract(outer_mask, contour_mask)

    inner_pixels = inner_mask == 255
    ring_pixels = ring_mask == 255

    if np.count_nonzero(inner_pixels) == 0 or np.count_nonzero(ring_pixels) == 0:
        return False

    inner_s = float(np.mean(hsv[:, :, 1][inner_pixels]))
    inner_v = float(np.mean(hsv[:, :, 2][inner_pixels]))
    ring_s = float(np.mean(hsv[:, :, 1][ring_pixels]))
    ring_v = float(np.mean(hsv[:, :, 2][ring_pixels]))

    value_delta = inner_v - ring_v
    saturation_delta = inner_s - ring_s

    return (
        value_delta >= MIN_VALUE_DELTA_TO_BACKGROUND
        and saturation_delta >= MIN_SATURATION_DELTA_TO_BACKGROUND
    )


def find_best_rectangle(roi_bgr):
    gray = cv2.cvtColor(roi_bgr, cv2.COLOR_BGR2GRAY)
    hsv = cv2.cvtColor(roi_bgr, cv2.COLOR_BGR2HSV)
    roi_height, roi_width = gray.shape[:2]

    object_mask = cv2.inRange(
        hsv,
        (0, MIN_COLOR_SATURATION, MIN_OBJECT_VALUE),
        (179, 255, 255),
    )

    kernel = np.ones((7, 7), np.uint8)
    object_mask = cv2.morphologyEx(object_mask, cv2.MORPH_OPEN, kernel)
    object_mask = cv2.morphologyEx(object_mask, cv2.MORPH_CLOSE, kernel)
    object_mask = cv2.erode(
        object_mask,
        np.ones((3, 3), np.uint8),
        iterations=OBJECT_MASK_ERODE_ITERATIONS,
    )

    contours, _ = cv2.findContours(
        object_mask,
        cv2.RETR_EXTERNAL,
        cv2.CHAIN_APPROX_SIMPLE,
    )

    best = None

    for contour in contours:
        area = cv2.contourArea(contour)
        if area < MIN_OBJECT_AREA_PX:
            continue

        rect = cv2.minAreaRect(contour)
        width_px, height_px = rect[1]
        if width_px <= 0 or height_px <= 0:
            continue

        longer_px = max(width_px, height_px)
        shorter_px = min(width_px, height_px)
        if shorter_px < MIN_OBJECT_SHORT_SIDE_PX:
            continue

        if longer_px > MAX_OBJECT_SIDE_PX or shorter_px > MAX_OBJECT_SIDE_PX:
            continue

        rect_area = width_px * height_px
        rectangularity = area / rect_area if rect_area > 0 else 0
        if rectangularity < MIN_RECTANGULARITY:
            continue

        x, y, width, height = cv2.boundingRect(contour)
        if contour_touches_roi_border(x, y, width, height, roi_width, roi_height):
            continue

        if not has_black_border(gray, x, y, width, height):
            continue

        if not has_colored_inside(hsv, contour):
            continue

        if not has_object_background_contrast(hsv, gray, contour, x, y, width, height):
            continue

        if best is None or area > best["area"]:
            best = {
                "area": area,
                "rect": rect,
                "longer_px": longer_px,
                "shorter_px": shorter_px,
            }

    return best


def measure_rectangle(roi_bgr):
    best = find_best_rectangle(roi_bgr)

    if best is None:
        return ""

    longer_mm = best["longer_px"] * MM_PER_PIXEL
    shorter_mm = best["shorter_px"] * MM_PER_PIXEL
    display_label = f"{longer_mm:.1f}x{shorter_mm:.1f} mm"
    result_payload = f"{longer_mm:.1f},{shorter_mm:.1f}"

    annotated = roi_bgr.copy()
    box = cv2.boxPoints(best["rect"])
    box = box.astype(np.int32)
    cv2.drawContours(annotated, [box], 0, (0, 255, 0), 3)
    cv2.putText(
        annotated,
        display_label,
        (max(10, int(box[:, 0].min())), max(30, int(box[:, 1].min()) - 10)),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (0, 255, 0),
        2,
        cv2.LINE_AA,
    )
    save_roi_image(annotated)

    return result_payload


def annotate_rectangle_center_offset(roi_bgr):
    best = find_best_rectangle(roi_bgr)
    annotated = roi_bgr.copy()
    roi_height, roi_width = annotated.shape[:2]
    image_center_y = roi_height // 2

    cv2.line(
        annotated,
        (0, image_center_y),
        (roi_width - 1, image_center_y),
        (255, 0, 0),
        2,
    )

    if best is None:
        cv2.putText(
            annotated,
            "UNRECOGNIZED",
            (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 0, 255),
            2,
            cv2.LINE_AA,
        )
        save_roi_image(annotated)
        return

    center_x, center_y = best["rect"][0]
    center_x = int(round(center_x))
    center_y = int(round(center_y))
    center_x = int(np.clip(center_x, 0, roi_width - 1))
    center_y = int(np.clip(center_y, 0, roi_height - 1))

    offset_px = center_y - image_center_y
    offset_mm = offset_px * MM_PER_PIXEL

    cv2.circle(annotated, (center_x, center_y), 8, (0, 0, 255), -1)
    cv2.arrowedLine(
        annotated,
        (center_x, center_y),
        (center_x, image_center_y),
        (0, 255, 255),
        2,
        tipLength=0.08,
    )

    label = f"dy={offset_px:+d} px / {offset_mm:+.1f} mm"
    label_x = min(max(10, center_x + 10), roi_width - 260)
    label_y = min(max(30, min(center_y, image_center_y) + 30), roi_height - 10)

    cv2.putText(
        annotated,
        label,
        (label_x, label_y),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.7,
        (0, 255, 255),
        2,
        cv2.LINE_AA,
    )

    save_roi_image(annotated)


picam2 = Picamera2()
config = picam2.create_preview_configuration(
    main={"size": (1280, 720), "format": "RGB888"}
)
picam2.configure(config)
picam2.set_controls({
    "AwbEnable": True,
})
picam2.start()
time.sleep(1.0)


@app.route("/read_qr", methods=["GET"])
def read_qr():
    try:
        roi_bgr = capture_and_save_roi()
        qr_data, qr_points = decode_qr(roi_bgr)

        if qr_data:
            annotate_qr(roi_bgr, qr_data, qr_points)
            return Response(qr_data, mimetype="text/plain")

        return Response("UNRECOGNIZED", mimetype="text/plain")

    except Exception:
        return Response("ERROR", mimetype="text/plain"), 500


@app.route("/check_size", methods=["GET"])
def check_size():
    try:
        roi_bgr = capture_and_save_roi()
        size_data = measure_rectangle(roi_bgr)

        if size_data:
            return Response(size_data, mimetype="text/plain")

        return Response("UNRECOGNIZED", mimetype="text/plain")

    except Exception:
        return Response("ERROR", mimetype="text/plain"), 500


@app.route("/check_center_offset", methods=["GET"])
def check_center_offset():
    try:
        roi_bgr = capture_and_save_roi()
        annotate_rectangle_center_offset(roi_bgr)
        return Response(status=204)

    except Exception:
        return Response("ERROR", mimetype="text/plain"), 500


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, threaded=False)
