using Microsoft.EntityFrameworkCore;

namespace LoginWebServer.Models
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options) : base(options) { }

        public DbSet<AccountDb> Accounts { get; set; }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            modelBuilder.Entity<AccountDb>(entity =>
            {
                entity.HasKey(e => e.AccountId);
                entity.HasIndex(e => e.AccountName).IsUnique();
                entity.Property(e => e.AccountName).HasMaxLength(20).IsRequired();
                entity.Property(e => e.PasswordHash).IsRequired();
            });
        }
    }
}
