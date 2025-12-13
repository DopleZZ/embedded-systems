INSERT INTO users (user_id, user_name, display_name, password_hash) VALUES
    (1, 'montana', 'Montana', '$2y$10$O3/yf5ZjKFVjB6IsOewbV.6XZar4FlXgladt0a1IVE7/27P8F0saO'),
    (2, 'nikola', 'Nikola Tesla', '$2y$10$BZ823gqXZv4ROEp.tXb46e//ag01G0VavvBmUKDHkk0jl1ahBiLqy'),
    (3, 'ada', 'Ada Lovelace', '$2y$10$g04k1AaxUb4A2vL1unOSQOjNP0f7dMf6Z.xQ238HsNyF624LpMVXO')
ON CONFLICT DO NOTHING;


INSERT INTO plant_states (plant_id, device_uid, owner_id, nickname, mood, friend_visible,
                          air_temperature_c, air_humidity_percent,
                          soil_moisture_percent, soil_moisture_raw, measurement_timestamp)
VALUES
    (1, 'esp32-beta', 2, 'Монстера Ника', 'THIRSTY', true, 25.0, 38.0, 21.0, 3700, NOW() - INTERVAL '20 minutes'),
    (2, 'esp32-gamma', 2, 'Суккулент Ада', 'NORMAL', false,  21.0, 55.0, 60.0, 2500, NOW() - INTERVAL '5 minutes'),
    (3, 'esp32-lal345', 2, 'Монстера Даник', 'THIRSTY', true, 25.0, 38.0, 21.0, 3700, NOW() - INTERVAL '15 minutes'),
    (4, 'esp32-048308587FB8', 4, 'Фикус Монго', 'HAPPY', true,  23.4, 48.0, 42.5, 3120, NOW() - INTERVAL '10 minutes')


    ON CONFLICT (device_uid) DO NOTHING;
