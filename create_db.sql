CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE genre (
	genre_id uuid DEFAULT uuid_generate_v1() PRIMARY KEY,
	name TEXT NOT NULL,
);

CREATE TABLE artist (
	artist_id uuid DEFAULT uuid_generate_v1() PRIMARY KEY,
	name TEXT NOT NULL
);

CREATE TABLE producer (
	producer_id uuid DEFAULT uuid_generate_v1() PRIMARY KEY,
	name TEXT NOT NULL
);

CREATE TABLE label (
	label_id uuid DEFAULT uuid_generate_v1() PRIMARY KEY,
	name TEXT NOT NULL
);

CREATE TABLE recording (
	recording_id uuid DEFAULT uuid_generate_v1() PRIMARY KEY,
	name TEXT,	
	release_date DATE
);

CREATE TABLE b_artist_genre (
	artist_id uuid REFERENCES artist (artist_id) ON UPDATE CASCADE ON DELETE CASCADE,
	genre_id uuid REFERENCES genre (genre_id) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT b_artist_genre_pkey PRIMARY KEY (artist_id, genre_id)
);

CREATE TABLE b_recording_label (
	recording_id uuid REFERENCES recording (recording_id) ON UPDATE CASCADE ON DELETE CASCADE,
	label_id uuid REFERENCES label (label_id) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT b_recording_label_pkey PRIMARY KEY (recording_id, label_id)
);

CREATE TABLE b_recording_producer (
	recording_id uuid REFERENCES recording (recording_id) ON UPDATE CASCADE ON DELETE CASCADE,
	producer_id uuid REFERENCES producer (producer_id) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT b_recording_producer_pkey PRIMARY KEY (recording_id, producer_id)
);

CREATE TABLE b_recording_genre (
	recording_id uuid REFERENCES recording (recording_id) ON UPDATE CASCADE ON DELETE CASCADE,
	genre_id uuid REFERENCES genre (genre_id) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT b_recording_genre_pkey PRIMARY KEY (recording_id, genre_id)
);

CREATE TABLE b_recording_artist (
	recording_id uuid REFERENCES recording (recording_id) ON UPDATE CASCADE ON DELETE CASCADE,
	artist_id uuid REFERENCES artist (artist_id) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT b_recording_artist_pkey PRIMARY KEY (recording_id, artist_id)
);
