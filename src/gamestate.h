#pragma once
class gamestate_integration
{
	public:

		/**
		* Initializes Game State Integration. Loads initial keybinds, sets everything to default state
		*/
		void init();
	
		void update_keybinds();

        input_context gsi_input_context;
	
        std::string gsi_current_input_categpory;

		void update_input_context();

		void update_player_stats();


	private:
		/**
		* Writes out the GSI JSON struct by preferred route.
		*/
		void write_out();

};


