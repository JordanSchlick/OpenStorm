
#include <string>
#include <vector>


namespace pfd
{
	enum class button
	{
		cancel = -1,
		ok,
		yes,
		no,
		abort,
		retry,
		ignore,
	};

	enum class choice
	{
		ok = 0,
		ok_cancel,
		yes_no,
		yes_no_cancel,
		retry_cancel,
		abort_retry_ignore,
	};

	enum class icon
	{
		info = 0,
		warning,
		error,
		question,
	};

	// Additional option flags for various dialog constructors
	enum class opt : uint8_t
	{
		none = 0,
		// For file open, allow multiselect.
		multiselect     = 0x1,
		// For file save, force overwrite and disable the confirmation dialog.
		force_overwrite = 0x2,
		// For folder select, force path to be the provided argument instead
		// of the last opened directory, which is the Microsoft-recommended,
		// user-friendly behaviour.
		force_path      = 0x4,
	};

	inline opt operator |(opt a, opt b) { return opt(uint8_t(a) | uint8_t(b)); }
	inline bool operator &(opt a, opt b) { return bool(uint8_t(a) & uint8_t(b)); }

	// The settings class, only exposing to the user a way to set verbose mode
	// and to force a rescan of installed desktop helpers (zenity, kdialog…).
	class settings
	{
	public:
		static bool available();

		static void verbose(bool value);
		static void rescan();

	protected:
		explicit settings(bool resync = false);

		bool check_program(std::string const &program);

		inline bool is_osascript() const;
		inline bool is_zenity() const;
		inline bool is_kdialog() const;

		enum class flag
		{
			is_scanned = 0,
			is_verbose,

			has_zenity,
			has_matedialog,
			has_qarma,
			has_kdialog,
			is_vista,

			max_flag,
		};

		// Static array of flags for internal state
		bool const &flags(flag in_flag) const;

		// Non-const getter for the static array of flags
		bool &flags(flag in_flag);
	};
	
	
	class open_file;
	
	class public_open_file{
	public:
		public_open_file(std::string const &title,
			std::string const &default_path = "",
			std::vector<std::string> const &filters = { "All Files", "*" },
			opt options = opt::none);
		~public_open_file();
		bool ready(int timeout = 0) const;
    	bool kill() const;
		std::vector<std::string> result();
		
		open_file* ptr = NULL;
	};
}