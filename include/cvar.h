typedef struct cvar_s
{
	char		*string;
	char		*name;
	float		value;
	struct cvar_s	*next;
} cvar_t;
