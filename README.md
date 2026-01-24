Home assistant mqtt.yaml
```
light:
  - name: "Light (kitchen set in kitchen)"
    unique_id: unique_id
    brightness_command_topic: "TOPIC_NAME/set"
    brightness_state_topic: "TOPIC_NAME/state"
    brightness_scale: 255
    on_command_type: "brightness"
    command_topic: "TOPIC_NAME/set"
    schema: basic
```