module.exports = [
  {
    "type": "heading",
    "defaultValue": "Dragon Radar Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Radar Style"
      },
      {
        "type": "radiogroup",
        "messageKey": "RADAR_STYLE",
        "label": "Radar Style",
        "defaultValue": "0",
        "options": [
          {
            "label": "4 Arrows (Edge Triangles)",
            "value": "0"
          },
          {
            "label": "1 Arrow (Center)",
            "value": "1"
          }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
