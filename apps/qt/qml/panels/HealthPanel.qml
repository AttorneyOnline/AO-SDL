import QtQuick
import QtQuick.Controls

/**
 * Health bar panel.  Binds defHp / proHp from the courtroom controller
 * down into the pure HealthBarDisplay component.
 */
HealthBarDisplay {
    id: root
    required property var controller

    defHp: controller ? controller.defHp : 0
    proHp: controller ? controller.proHp : 0
}
