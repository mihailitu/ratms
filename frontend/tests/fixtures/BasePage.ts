import { Page, Locator } from '@playwright/test';

/**
 * Base Page Object with common functionality
 */
export class BasePage {
  readonly page: Page;

  constructor(page: Page) {
    this.page = page;
  }

  /**
   * Navigate to a specific path
   */
  async goto(path: string = '/') {
    await this.page.goto(path);
  }

  /**
   * Get navigation link
   */
  getNavLink(text: string): Locator {
    return this.page.getByRole('link', { name: text });
  }

  /**
   * Navigate using the main navigation
   */
  async navigateTo(linkText: string) {
    await this.getNavLink(linkText).click();
    await this.page.waitForLoadState('networkidle');
  }

  /**
   * Check if we're on a specific route
   */
  async isOnRoute(path: string): Promise<boolean> {
    const url = this.page.url();
    return url.includes(path);
  }

  /**
   * Wait for page to be fully loaded
   */
  async waitForPageLoad() {
    await this.page.waitForLoadState('networkidle');
  }

  /**
   * Get error message if displayed
   */
  getErrorMessage(): Locator {
    return this.page.locator('text=/error:/i').first();
  }

  /**
   * Get loading indicator
   */
  getLoadingIndicator(): Locator {
    return this.page.getByText(/loading/i);
  }

  /**
   * Check if page has error
   */
  async hasError(): Promise<boolean> {
    try {
      await this.getErrorMessage().waitFor({ timeout: 1000 });
      return true;
    } catch {
      return false;
    }
  }

  /**
   * Check if page is loading
   */
  async isLoading(): Promise<boolean> {
    try {
      await this.getLoadingIndicator().waitFor({ timeout: 1000 });
      return true;
    } catch {
      return false;
    }
  }
}
