INSERT INTO users (user_id, user_name, display_name) VALUES
    (1, 'montana', 'Montana'),
    (2, 'nikola', 'Nikola Tesla'),
    (3, 'ada', 'Ada Lovelace')
ON CONFLICT DO NOTHING;


INSERT INTO plant_states (plant_id, device_uid, owner_id, nickname, mood, friend_visible,
                          air_temperature_c, air_humidity_percent,
                          soil_moisture_percent, soil_moisture_raw, measurement_timestamp)
VALUES
    (1, 'esp32-048308587FB8', 1, 'Фикус Монго', 'HAPPY', true,  23.4, 48.0, 42.5, 3120, NOW() - INTERVAL '10 minutes'),
    (2, 'esp32-beta', 2, 'Монстера Ника', 'THIRSTY', true, 25.0, 38.0, 21.0, 3700, NOW() - INTERVAL '20 minutes'),
    (3, 'esp32-gamma', 2, 'Суккулент Ада', 'NORMAL', false,  21.0, 55.0, 60.0, 2500, NOW() - INTERVAL '5 minutes'),
    (4, 'esp32-lal345', 2, 'Монстера Даник', 'THIRSTY', true, 25.0, 38.0, 21.0, 3700, NOW() - INTERVAL '15 minutes'),
    (5, 'esp32-048308587FB8', 1, 'Фикус Монго', 'HAPPY', true,  23.4, 48.0, 42.5, 3120, NOW() - INTERVAL '10 minutes')

ON CONFLICT (device_uid) DO NOTHING;
