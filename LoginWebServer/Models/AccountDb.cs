namespace LoginWebServer.Models
{
    public class AccountDb
    {
        public int AccountId { get; set; }
        public string AccountName { get; set; } = string.Empty;
        public string PasswordHash { get; set; } = string.Empty;
        public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    }
}
